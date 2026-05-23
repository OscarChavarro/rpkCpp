import { Matrix4x4d } from "../../common/linealAlgebra/Matrix4x4d";
import { Vector3Dd } from "../../common/linealAlgebra/Vector3Dd";
import { EntityTypeContext } from "../context/EntityTypeContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { TokenValidationContext } from "../context/TokenValidationContext";
import { TransformArrayContext } from "../context/TransformArrayContext";
import { TransformContext } from "../context/TransformContext";
import { TransformScopeContext } from "../context/TransformScopeContext";
import { TransformSequenceContext } from "../context/TransformSequenceContext";
import { TransformStackContext } from "../context/TransformStackContext";
import { MgfEntityControl } from "./MgfEntityControl";

/**
Routines for 4x4 homogeneous, rigid-body transformations.
*/
export class MgfTransformationSupport {
  private constructor() {
  }

  /**
  Compute unique ID from matrix.
  */
  private static computeUniqueId(xfm: Matrix4x4d): number {
    const shiftTab = [
      15, 5, 11, 5, 6, 3, 9, 15,
      13, 2, 13, 5, 2, 12, 14, 11,
      11, 12, 12, 3, 2, 11, 8, 12,
      1, 12, 5, 4, 15, 9, 14, 5,
      13, 14, 2, 10, 10, 14, 12, 3,
      5, 5, 14, 6, 12, 11, 13, 9,
      12, 8, 1, 6, 5, 12, 7, 13,
      15, 8, 9, 2, 6, 11, 9, 11,
    ];
    let xid = 0;

    // Compute unique transform id
    const buffer = new ArrayBuffer(16 * 8);
    const view = new DataView(buffer);
    let offset = 0;
    for (let r = 0; r < 4; r++) {
      const row = xfm.m[r]!;
      for (let c = 0; c < 4; c++) {
        view.setFloat64(offset, row[c]!, true);
        offset += 8;
      }
    }
    const raw = new Uint8Array(buffer);
    for (let i = 0; i + 1 < raw.length; i += 2) {
      const value = (raw[i]! & 0xff) | ((raw[i + 1]! & 0xff) << 8);
      xid ^= (value << shiftTab[(i / 2) & 63]!);
    }
    return xid;
  }

  private static d2r(a: number): number {
    return (globalThis.Math.PI / 180.0) * a;
  }

  /**
  Check argument list against format string.
  */
  private static checkForBadArguments(ac: number, av: string[], fl: string | null): number {
    const format = fl ?? "";
    for (let formatIndex = 0; formatIndex < format.length; formatIndex++) {
      const argumentIndex = formatIndex + 1;
      const token = av[formatIndex];
      if (argumentIndex > ac || token === undefined) {
        return -1;
      }
      switch (format.charAt(formatIndex)) {
        case "s": // String
          if (token.length <= 0 || /\s/.test(token.charAt(0))) {
            return argumentIndex;
          }
          break;
        case "i": // Integer
          if (!TokenValidationContext.isIntDelimited(token, " \t\r\n")) {
            return argumentIndex;
          }
          break;
        case "f": // Float
          if (!TokenValidationContext.isFloatDelimited(token, " \t\r\n")) {
            return argumentIndex;
          }
          break;
        default:
          return -1;
      }
    }
    return 0; // All's well
  }

  private static checkArgument(a: number, l: string, ac: number, av: string[], i: number): boolean {
    if (av[i] === undefined || av[i]!.length !== a) {
      return false;
    }
    const tail = av.slice(i + 1, ac);
    if (MgfTransformationSupport.checkForBadArguments(ac - i - 1, tail, l) !== 0) {
      return false;
    }
    return true;
  }

  /**
  Put out name for this instance.
  */
  private static transformName(ap: TransformSequenceContext | null, context: ParseRuntimeContext): number {
    const oav: string[] = [
      context.entityNames[EntityTypeContext.OBJECT] ?? "",
      "",
    ];
    if (ap === null) {
      return MgfEntityControl.mgfHandle(EntityTypeContext.OBJECT, 1, oav, context);
    }
    let oName = "a";
    for (let i = 0; i < ap.numberOfDimensions; i++) {
      oName += `${ap.transformArguments[i]!.arg}.`;
    }
    oav[1] = oName;
    return MgfEntityControl.mgfHandle(EntityTypeContext.OBJECT, 2, oav, context);
  }

  /**
  Allocate new transform structure.
  */
  private static newTransform(ac: number, av: string[], context: ParseRuntimeContext): TransformStackContext | null {
    const stack = context.transformStack;
    let nDim = 0;
    const previousArgumentCount = stack.argumentCountFor(context.transformContext);

    // Compute space required by arguments
    for (let i = 0; i < ac; i++) {
      if (av[i] === "-a") {
        nDim++;
        i++;
      }
    }
    if (nDim > TransformSequenceContext.TRANSFORM_MAXIMUM_DIMENSIONS) {
      return null;
    }

    const spec = new TransformStackContext();
    spec.ownedArgumentCount = ac;
    if (ac > 0) {
      spec.ownedArgumentCopies = new Array<string | null>(ac).fill(null);
    }

    if (nDim !== 0) {
      spec.transformationArray = new TransformSequenceContext();
      const fp = spec.transformationArray.startingPosition;
      MgfEntityControl.mgfGetFilePosition(fp, context);
      spec.transformationArray.numberOfDimensions = 0;
    }
    else {
      spec.transformationArray = null;
    }
    spec.xac = ac + previousArgumentCount;

    // Allocate argument list with new arguments first and inherited after.
    let newArgumentList: Array<string | null> | null = null;
    if (spec.xac > 0) {
      newArgumentList = new Array<string | null>(spec.xac + 1).fill(null);

      const previousStartIndex = stack.argumentCount - previousArgumentCount;
      for (let i = 0; i < previousArgumentCount; i++) {
        newArgumentList[ac + i] = (stack.argumentList as Array<string | null>)[previousStartIndex + i]!;
      }
      newArgumentList[spec.xac] = null;
    }
    stack.argumentList = newArgumentList;
    stack.argumentCount = spec.xac;

    // Use memory allocated above
    for (let i = 0; i < ac; i++) {
      if (av[i] === "-a") {
        (stack.argumentList as Array<string | null>)[i] = stack.iterateArgument;
        (spec.ownedArgumentCopies as Array<string | null>)[i] = null;
        i++;
        const transformArgument = (spec.transformationArray as TransformSequenceContext).transformArguments[
          (spec.transformationArray as TransformSequenceContext).numberOfDimensions
        ] as TransformArrayContext;
        transformArgument.arg = "0";
        transformArgument.argumentIndex = i;
        (spec.ownedArgumentCopies as Array<string | null>)[i] = null;
        (stack.argumentList as Array<string | null>)[i] = transformArgument.arg;
        transformArgument.i = 0;
        transformArgument.n = Number.parseInt(av[i]!, 10);
        (spec.transformationArray as TransformSequenceContext).numberOfDimensions++;
      }
      else {
        const argumentCopy = av[i]!;
        (stack.argumentList as Array<string | null>)[i] = argumentCopy;
        (spec.ownedArgumentCopies as Array<string | null>)[i] = argumentCopy;
      }
    }
    return spec;
  }

  /**
  Transform a point by the current matrix.
  */
  public static mgfTransformPoint(v1: Vector3Dd, v2: Vector3Dd, context: ParseRuntimeContext): void {
    if (context.transformContext === null) {
      v1.copy(v2);
      return;
    }
    context.transformContext.xf.transformMatrix.multiplyWithTranslation(v1, v2);
  }

  /**
  Transform a vector using current matrix.
  */
  public static mgfTransformVector(v1: Vector3Dd, v2: Vector3Dd, context: ParseRuntimeContext): void {
    if (context.transformContext === null) {
      v1.copy(v2);
      return;
    }
    context.transformContext.xf.transformMatrix.multiply(v1, v2);
  }

  private static finish(count: number, ret: TransformContext, transformMatrix: Matrix4x4d, scaTransform: number): void {
    while (count-- > 0) {
      Matrix4x4d.multiplyMatrix4(ret.transformMatrix, ret.transformMatrix, transformMatrix);
      ret.scaleFactor *= scaTransform;
    }
  }

  /**
  Get transform specification.
  */
  private static xf(ret: TransformContext, ac: number, av: string[] | null): number {
    ret.transformMatrix.identity();
    ret.scaleFactor = 1.0;

    if (av === null) {
      return 0;
    }

    let counter = 1;
    const transformMatrix = new Matrix4x4d();
    transformMatrix.identity();
    let scaTransform = 1.0;

    let i: number;
    let tmp: number;
    for (i = 0; i < ac; i++) {
      const cmd = av[i];
      if (cmd === undefined || cmd === null || cmd.length <= 0 || cmd.charAt(0) !== "-") {
        break;
      }
      const m4 = new Matrix4x4d();

      if (cmd.length < 2) {
        MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
        return i;
      }

      switch (cmd.charAt(1)) {
        case "t":
          // Translate
          if (!MgfTransformationSupport.checkArgument(2, "fff", ac, av, i)) {
            MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
            return i;
          }
          m4.m[3]![0] = Number.parseFloat(av[++i]!);
          m4.m[3]![1] = Number.parseFloat(av[++i]!);
          m4.m[3]![2] = Number.parseFloat(av[++i]!);
          break;

        case "r": {
          // Rotate
          const suffix = cmd.length > 2 ? cmd.charAt(2) : "\0";
          switch (suffix) {
            case "x":
              if (!MgfTransformationSupport.checkArgument(3, "f", ac, av, i)) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              tmp = MgfTransformationSupport.d2r(Number.parseFloat(av[++i]!));
              m4.m[1][1] = m4.m[2][2] = globalThis.Math.cos(tmp);
              m4.m[2][1] = -(m4.m[1][2] = globalThis.Math.sin(tmp));
              break;
            case "y":
              if (!MgfTransformationSupport.checkArgument(3, "f", ac, av, i)) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              tmp = MgfTransformationSupport.d2r(Number.parseFloat(av[++i]!));
              m4.m[0][0] = m4.m[2][2] = globalThis.Math.cos(tmp);
              m4.m[0][2] = -(m4.m[2][0] = globalThis.Math.sin(tmp));
              break;
            case "z":
              if (!MgfTransformationSupport.checkArgument(3, "f", ac, av, i)) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              tmp = MgfTransformationSupport.d2r(Number.parseFloat(av[++i]!));
              m4.m[0][0] = m4.m[1][1] = globalThis.Math.cos(tmp);
              m4.m[1][0] = -(m4.m[0][1] = globalThis.Math.sin(tmp));
              break;
            default: {
              if (!MgfTransformationSupport.checkArgument(2, "ffff", ac, av, i)) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              let x = Number.parseFloat(av[++i]!);
              let y = Number.parseFloat(av[++i]!);
              let z = Number.parseFloat(av[++i]!);
              const a = MgfTransformationSupport.d2r(Number.parseFloat(av[++i]!));
              let s = globalThis.Math.sqrt(x * x + y * y + z * z);
              x /= s;
              y /= s;
              z /= s;
              const c = globalThis.Math.cos(a);
              s = globalThis.Math.sin(a);
              const t = 1 - c;
              m4.m[0][0] = t * x * x + c;
              m4.m[1][1] = t * y * y + c;
              m4.m[2][2] = t * z * z + c;
              let A = t * x * y;
              let B = s * z;
              m4.m[0][1] = A + B;
              m4.m[1][0] = A - B;
              A = t * x * z;
              B = s * y;
              m4.m[0][2] = A - B;
              m4.m[2][0] = A + B;
              A = t * y * z;
              B = s * x;
              m4.m[1][2] = A + B;
              m4.m[2][1] = A - B;
            }
          }
          break;
        }

        case "s": {
          // Scale
          const suffix = cmd.length > 2 ? cmd.charAt(2) : "\0";
          switch (suffix) {
            case "x":
              if (!MgfTransformationSupport.checkArgument(3, "f", ac, av, i)) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              tmp = Number.parseFloat(av[i + 1]!);
              if (tmp === 0.0) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              m4.m[0][0] = tmp;
              break;
            case "y":
              if (!MgfTransformationSupport.checkArgument(3, "f", ac, av, i)) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              tmp = Number.parseFloat(av[i + 1]!);
              if (tmp === 0.0) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              m4.m[1][1] = tmp;
              break;
            case "z":
              if (!MgfTransformationSupport.checkArgument(3, "f", ac, av, i)) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              tmp = Number.parseFloat(av[i + 1]!);
              if (tmp === 0.0) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              m4.m[2][2] = tmp;
              break;
            default:
              if (!MgfTransformationSupport.checkArgument(2, "f", ac, av, i)) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              tmp = Number.parseFloat(av[i + 1]!);
              if (tmp === 0.0) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              scaTransform *= (m4.m[0][0] = m4.m[1][1] = m4.m[2][2] = tmp);
              break;
          }
          i++;
          break;
        }

        case "m": {
          // Mirror
          const suffix = cmd.length > 2 ? cmd.charAt(2) : "\0";
          switch (suffix) {
            case "x":
              if (!MgfTransformationSupport.checkArgument(3, "", ac, av, i)) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              scaTransform *= (m4.m[0][0] = -1.0);
              break;
            case "y":
              if (!MgfTransformationSupport.checkArgument(3, "", ac, av, i)) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              scaTransform *= (m4.m[1][1] = -1.0);
              break;
            case "z":
              if (!MgfTransformationSupport.checkArgument(3, "", ac, av, i)) {
                MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
                return i;
              }
              scaTransform *= (m4.m[2][2] = -1.0);
              break;
            default:
              MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
              return i;
          }
          break;
        }

        case "i":
          // Iterate
          if (!MgfTransformationSupport.checkArgument(2, "i", ac, av, i)) {
            MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
            return i;
          }
          while (counter-- > 0) {
            Matrix4x4d.multiplyMatrix4(ret.transformMatrix, ret.transformMatrix, transformMatrix);
            ret.scaleFactor *= scaTransform;
          }
          counter = Number.parseInt(av[++i]!, 10);
          transformMatrix.identity();
          scaTransform = 1.0;
          continue;

        default:
          MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
          return i;
      }
      Matrix4x4d.multiplyMatrix4(transformMatrix, transformMatrix, m4);
    }

    MgfTransformationSupport.finish(counter, ret, transformMatrix, scaTransform);
    return i;
  }

  private static compactTransformArguments(context: ParseRuntimeContext, stackContext: TransformStackContext | null): boolean {
    return context.transformStack.compactTo(stackContext);
  }

  /**
  Handle xf entity.
  */
  public static handleTransformationEntity(ac: number, av: string[], context: ParseRuntimeContext): number {
    const stack = context.transformStack;
    let spec: TransformStackContext | null;
    let n: number;

    if (ac === 1) {
      // Something with existing transform
      spec = context.transformContext;
      if (spec === null) {
        return ParseErrorContext.MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
      }
      n = -1;
      if (spec.transformationArray !== null) {
        // Check for iteration
        const ap = spec.transformationArray;

        MgfTransformationSupport.transformName(null, context);
        n = ap.numberOfDimensions;
        while (n-- > 0) {
          const transformArgument = ap.transformArguments[n]!;
          if (++transformArgument.i < transformArgument.n) {
            break;
          }
          transformArgument.arg = "0";
          if (transformArgument.argumentIndex >= 0
            && transformArgument.argumentIndex < stack.argumentCount) {
            (stack.argumentList as Array<string | null>)[transformArgument.argumentIndex] = transformArgument.arg;
          }
          transformArgument.i = 0;
        }
        if (n >= 0) {
          const rv = MgfEntityControl.mgfGoToFilePosition(ap.startingPosition, context);
          if (rv !== ParseErrorContext.MGF_OK) {
            return rv;
          }
          const transformArgument = ap.transformArguments[n]!;
          transformArgument.arg = `${transformArgument.i}`;
          if (transformArgument.argumentIndex >= 0
            && transformArgument.argumentIndex < stack.argumentCount) {
            (stack.argumentList as Array<string | null>)[transformArgument.argumentIndex] = transformArgument.arg;
          }
          MgfTransformationSupport.transformName(ap, context);
        }
      }
      if (n < 0) {
        // Pop transform
        if (!MgfTransformationSupport.compactTransformArguments(context, spec.prev)) {
          return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
        }
        context.transformContext = spec.prev;
        context.transformStack.transformContext = context.transformContext;
        context.transformStack.freeTransformContext(spec);
        return ParseErrorContext.MGF_OK;
      }
    }
    else {
      // Allocate transform
      const slice = av.slice(1, ac);
      spec = MgfTransformationSupport.newTransform(ac - 1, slice, context);
      if (spec === null) {
        return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
      }
      if (spec.transformationArray !== null) {
        MgfTransformationSupport.transformName(spec.transformationArray, context);
      }
      spec.prev = context.transformContext; // Push onto stack
      context.transformContext = spec;
      context.transformStack.transformContext = context.transformContext;
    }

    // Translate new specification
    n = stack.argumentCountFor(spec);
    n -= stack.argumentCountFor(spec.prev); // Incremental comp is more efficient
    const specAv = stack.argumentVectorFor(spec) as string[] | null;
    if (MgfTransformationSupport.xf(spec.xf, n, specAv) !== n) {
      return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
    }

    // Check for vertex reversal
    spec.rev = spec.xf.scaleFactor < 0.0 ? 1 : 0;
    if (spec.rev !== 0) {
      spec.xf.scaleFactor = -spec.xf.scaleFactor;
    }

    // Compute total transformation
    if (spec.prev !== null) {
      Matrix4x4d.multiplyMatrix4(spec.xf.transformMatrix, spec.xf.transformMatrix, spec.prev.xf.transformMatrix);
      spec.xf.scaleFactor *= spec.prev.xf.scaleFactor;
      spec.rev = ((spec.rev ^ spec.prev.rev) !== 0) ? 1 : 0;
    }
    spec.xid = MgfTransformationSupport.computeUniqueId(spec.xf.transformMatrix);
    return ParseErrorContext.MGF_OK;
  }

  public static mgfTransformFreeMemory(context: ParseRuntimeContext): void {
    if (context !== null) {
      context.transformStack.clearArguments();
    }
  }
}

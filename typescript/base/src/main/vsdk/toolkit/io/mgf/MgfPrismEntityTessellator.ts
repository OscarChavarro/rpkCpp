import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3Dd } from "../../common/linealAlgebra/Vector3Dd";
import { EntityTypeContext } from "../context/EntityTypeContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { ReaderContext } from "../context/ReaderContext";
import { TokenValidationContext } from "../context/TokenValidationContext";
import { VertexContext } from "../context/VertexContext";
import { MgfEntityControl } from "./MgfEntityControl";
import { MgfTessellationMath } from "./MgfTessellationMath";
import { MgfVertexFaceEntitySupport } from "./MgfVertexFaceEntitySupport";

export class MgfPrismEntityTessellator {
  private constructor() {
  }

  /**
  Turn a prism into polygons
  */
  public static handleEntity(argumentCount: number, argumentValues: string[], context: ParseRuntimeContext): number {
    const px = [""];
    const py = [""];
    const pz = [""];
    const vertexEntity = [
      context.entityNames[EntityTypeContext.VERTEX],
      "",
      "=",
      "",
    ];
    const pointEntity = [
      context.entityNames[EntityTypeContext.MGF_POINT],
      "",
      "",
      "",
    ];
    const zeroNormal = [
      context.entityNames[EntityTypeContext.MGF_NORMAL],
      "0",
      "0",
      "0",
    ];
    const newArgumentValues = new Array<string>(ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT);
    const newVertexNames = new Array<string>(ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT - 1);
    let vertexContext: VertexContext | null;
    let errorCode: number;
    let i: number;

    // Check arguments
    if (argumentCount < 5) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if (!TokenValidationContext.isFloat(argumentValues[argumentCount - 1]!)) {
      return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
    }
    const length = Number.parseFloat(argumentValues[argumentCount - 1]!);
    if (length <= Numeric.EPSILON && length >= -Numeric.EPSILON) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Compute face normal
    const v0Context = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1]!, context);
    if (v0Context === null) {
      return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
    }
    let hasNormal = 0;

    const normal = new Vector3Dd(0.0, 0.0, 0.0);
    const v1 = new Vector3Dd(0.0, 0.0, 0.0);

    for (i = 2; i < argumentCount - 1; i++) {
      vertexContext = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[i]!, context);
      if (vertexContext === null) {
        return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
      }

      if (!vertexContext.n.isNull(Numeric.EPSILON)) {
        hasNormal++;
      }

      const v2 = new Vector3Dd();
      const v3 = new Vector3Dd();

      v2.x = vertexContext.p.x - v0Context.p.x;
      v2.y = vertexContext.p.y - v0Context.p.y;
      v2.z = vertexContext.p.z - v0Context.p.z;
      v3.crossProduct(v1, v2);
      normal.x += v3.x;
      normal.y += v3.y;
      normal.z += v3.z;
      v1.copy(v2);
    }
    if (normal.normalizeAndGivePreviousNorm(Numeric.EPSILON) === 0.0) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Create moved vertices
    for (i = 1; i < argumentCount - 1; i++) {
      newVertexNames[i - 1] = `_pv${i}`;
      vertexEntity[1] = newVertexNames[i - 1];
      vertexEntity[3] = argumentValues[i]!;
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, vertexEntity as string[], context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      vertexContext = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[i]!, context); // Checked above
      if (vertexContext === null) {
        return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
      }
      MgfTessellationMath.formatFloat(px, 24, vertexContext.p.x - length * normal.x);
      MgfTessellationMath.formatFloat(py, 24, vertexContext.p.y - length * normal.y);
      MgfTessellationMath.formatFloat(pz, 24, vertexContext.p.z - length * normal.z);
      pointEntity[1] = px[0]!;
      pointEntity[2] = py[0]!;
      pointEntity[3] = pz[0]!;
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, pointEntity as string[], context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
    }

    // Make faces
    newArgumentValues[0] = context.entityNames[EntityTypeContext.FACE]!;
    // Do the side faces
    newArgumentValues[5] = "";
    newArgumentValues[3] = argumentValues[argumentCount - 2]!;
    newArgumentValues[4] = newVertexNames[argumentCount - 3]!;
    for (i = 1; i < argumentCount - 1; i++) {
      newArgumentValues[1] = newVertexNames[i - 1]!;
      newArgumentValues[2] = argumentValues[i]!;
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.FACE, 5, newArgumentValues as string[], context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      newArgumentValues[3] = newArgumentValues[2];
      newArgumentValues[4] = newArgumentValues[1];
    }

    // Do top face
    for (i = 1; i < argumentCount - 1; i++) {
      if (hasNormal !== 0) {
        // Zero normals
        vertexEntity[1] = newVertexNames[i - 1]!;
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, vertexEntity as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, zeroNormal as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
      }
      // Reverse
      newArgumentValues[argumentCount - 1 - i] = newVertexNames[i - 1]!;
    }
    errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.FACE, argumentCount - 1, newArgumentValues as string[], context);
    if (errorCode !== ParseErrorContext.MGF_OK) {
      return errorCode;
    }

    // Do bottom face
    if (hasNormal !== 0) {
      for (i = 1; i < argumentCount - 1; i++) {
        vertexEntity[1] = newVertexNames[i - 1]!;
        vertexEntity[3] = argumentValues[i]!;
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, vertexEntity as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, zeroNormal as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        newArgumentValues[i] = newVertexNames[i - 1]!;
      }
    }
    else {
      for (i = 1; i < argumentCount - 1; i++) {
        newArgumentValues[i] = argumentValues[i]!;
      }
    }
    newArgumentValues[i] = "";
    errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.FACE, i, newArgumentValues as string[], context);
    if (errorCode !== ParseErrorContext.MGF_OK) {
      return errorCode;
    }
    return ParseErrorContext.MGF_OK;
  }
}

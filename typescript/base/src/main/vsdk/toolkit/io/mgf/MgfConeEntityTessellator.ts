import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3Dd } from "../../common/linealAlgebra/Vector3Dd";
import { EntityTypeContext } from "../context/EntityTypeContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { TokenValidationContext } from "../context/TokenValidationContext";
import { VertexContext } from "../context/VertexContext";
import { MgfEntityControl } from "./MgfEntityControl";
import { MgfTessellationMath } from "./MgfTessellationMath";
import { MgfVertexFaceEntitySupport } from "./MgfVertexFaceEntitySupport";

export class MgfConeEntityTessellator {
  private constructor() {
  }

  /**
  Turn a cone into polygons
  */
  public static handleEntity(argumentCount: number, argumentValues: string[], context: ParseRuntimeContext): number {
    const p3x = [""];
    const p3y = [""];
    const p3z = [""];
    const p4x = [""];
    const p4y = [""];
    const p4z = [""];
    const n3x = [""];
    const n3y = [""];
    const n3z = [""];
    const n4x = [""];
    const n4y = [""];
    const n4z = [""];
    const v1Entity = [
      context.entityNames[EntityTypeContext.VERTEX],
      "_cv1",
      "=",
      "",
    ];
    const v2Entity = [
      context.entityNames[EntityTypeContext.VERTEX],
      "_cv2",
      "=",
      "_cv3",
    ];
    const v3Entity = [
      context.entityNames[EntityTypeContext.VERTEX],
      "_cv3",
      "=",
    ];
    const p3Entity = [context.entityNames[EntityTypeContext.MGF_POINT], "", "", ""];
    const n3Entity = [context.entityNames[EntityTypeContext.MGF_NORMAL], "", "", ""];
    const v4Entity = [
      context.entityNames[EntityTypeContext.VERTEX],
      "_cv4",
      "=",
    ];
    const p4Entity = [
      context.entityNames[EntityTypeContext.MGF_POINT],
      "",
      "",
      "",
    ];
    const n4Entity = [
      context.entityNames[EntityTypeContext.MGF_NORMAL],
      "",
      "",
      "",
    ];
    const faceEntity = [
      context.entityNames[EntityTypeContext.FACE],
      "_cv1",
      "_cv2",
      "_cv3",
      "_cv4",
    ];
    let v1Name: string;
    let v1Context: VertexContext | null;
    let v2Context: VertexContext | null;
    let normalOffset1: number;
    let normalOffset2: number;
    let d: number;
    let errorCode: number;

    if (argumentCount !== 5) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    v1Context = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1], context);
    v2Context = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[3], context);
    if (v1Context === null || v2Context === null) {
      return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
    }
    v1Name = argumentValues[1];
    if (!TokenValidationContext.isFloat(argumentValues[2]) || !TokenValidationContext.isFloat(argumentValues[4])) {
      return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
    }

    // Set up (radius1, radius2)
    let radius1 = Number.parseFloat(argumentValues[2]);
    if (radius1 <= Numeric.EPSILON && radius1 >= -Numeric.EPSILON) {
      radius1 = 0.0;
    }
    let radius2 = Number.parseFloat(argumentValues[4]);
    if (radius2 <= Numeric.EPSILON && radius2 >= -Numeric.EPSILON) {
      radius2 = 0.0;
    }

    if (radius1 === 0.0) {
      if (radius2 === 0.0) {
        return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
      }
    }
    else if (radius2 !== 0.0) {
      const a = radius1 < 0.0;
      const b = radius2 < 0.0;
      const check = (a && !b) || (!a && b); // exclusive or
      if (check) {
        return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
      }
    }
    else {
      // Swap
      const swappedVertexContext = v1Context;
      v1Context = v2Context;
      v2Context = swappedVertexContext;
      v1Name = argumentValues[3];
      d = radius1;
      radius1 = radius2;
      radius2 = d;
    }
    const sign = radius2 < 0.0 ? -1 : 1;

    // Initialize
    const w = new Vector3Dd();
    w.x = v1Context.p.x - v2Context.p.x;
    w.y = v1Context.p.y - v2Context.p.y;
    w.z = v1Context.p.z - v2Context.p.z;

    d = w.normalizeAndGivePreviousNorm(Numeric.EPSILON);
    if (Numeric.doubleEqual(d, 0.0, Numeric.EPSILON)) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    normalOffset1 = normalOffset2 = (radius2 - radius1) / d;
    if (context.warpConeEnds) {
      // Hack for mgfEntitySphere and mgfEntityTorus
      d = globalThis.Math.atan(normalOffset2) - (globalThis.Math.PI / 4) / context.numberOfQuarterCircleDivisions;
      if (d <= -globalThis.Math.PI / 2 + Numeric.EPSILON) {
        normalOffset2 = -Numeric.HUGE_FLOAT_VALUE;
      }
      else {
        normalOffset2 = globalThis.Math.tan(d);
      }
    }

    const u = new Vector3Dd();
    const v = new Vector3Dd();
    MgfTessellationMath.mgfMakeAxes(u, v, w, Numeric.EPSILON);

    MgfTessellationMath.formatFloat(p3x, 24, v2Context.p.x + radius2 * u.x);
    if (normalOffset2 <= -Numeric.HUGE_FLOAT_VALUE) {
      MgfTessellationMath.formatFloat(n3x, 24, -w.x);
    }
    else {
      MgfTessellationMath.formatFloat(n3x, 24, u.x + w.x * normalOffset2);
    }

    MgfTessellationMath.formatFloat(p3y, 24, v2Context.p.y + radius2 * u.y);
    if (normalOffset2 <= -Numeric.HUGE_FLOAT_VALUE) {
      MgfTessellationMath.formatFloat(n3y, 24, -w.y);
    }
    else {
      MgfTessellationMath.formatFloat(n3y, 24, u.y + w.y * normalOffset2);
    }

    MgfTessellationMath.formatFloat(p3z, 24, v2Context.p.z + radius2 * u.z);
    if (normalOffset2 <= -Numeric.HUGE_FLOAT_VALUE) {
      MgfTessellationMath.formatFloat(n3z, 24, -w.z);
    }
    else {
      MgfTessellationMath.formatFloat(n3z, 24, u.z + w.z * normalOffset2);
    }

    p3Entity[1] = p3x[0];
    p3Entity[2] = p3y[0];
    p3Entity[3] = p3z[0];
    n3Entity[1] = n3x[0];
    n3Entity[2] = n3y[0];
    n3Entity[3] = n3z[0];

    errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 3, v3Entity, context);
    if (errorCode !== ParseErrorContext.MGF_OK) {
      return errorCode;
    }
    errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p3Entity, context);
    if (errorCode !== ParseErrorContext.MGF_OK) {
      return errorCode;
    }
    errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, n3Entity, context);
    if (errorCode !== ParseErrorContext.MGF_OK) {
      return errorCode;
    }
    if (radius1 === 0.0) {
      // Triangles
      v1Entity[3] = v1Name;
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v1Entity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }

      MgfTessellationMath.formatFloat(n4x, 24, w.x);
      MgfTessellationMath.formatFloat(n4y, 24, w.y);
      MgfTessellationMath.formatFloat(n4z, 24, w.z);
      n4Entity[1] = n4x[0];
      n4Entity[2] = n4y[0];
      n4Entity[3] = n4z[0];

      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, n4Entity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      for (let i = 1; i <= 4 * context.numberOfQuarterCircleDivisions; i++) {
        const theta = sign * i * (globalThis.Math.PI / 2) / context.numberOfQuarterCircleDivisions;
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v2Entity, context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }

        d = u.x * globalThis.Math.cos(theta) + v.x * globalThis.Math.sin(theta);
        MgfTessellationMath.formatFloat(p3x, 24, v2Context.p.x + radius2 * d);
        if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE) {
          MgfTessellationMath.formatFloat(n3x, 24, d + w.x * normalOffset2);
        }

        d = u.y * globalThis.Math.cos(theta) + v.y * globalThis.Math.sin(theta);
        MgfTessellationMath.formatFloat(p3y, 24, v2Context.p.y + radius2 * d);
        if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE) {
          MgfTessellationMath.formatFloat(n3y, 24, d + w.y * normalOffset2);
        }

        d = u.z * globalThis.Math.cos(theta) + v.z * globalThis.Math.sin(theta);
        MgfTessellationMath.formatFloat(p3z, 24, v2Context.p.z + radius2 * d);
        if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE) {
          MgfTessellationMath.formatFloat(n3z, 24, d + w.z * normalOffset2);
        }
        p3Entity[1] = p3x[0];
        p3Entity[2] = p3y[0];
        p3Entity[3] = p3z[0];
        n3Entity[1] = n3x[0];
        n3Entity[2] = n3y[0];
        n3Entity[3] = n3z[0];

        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v3Entity, context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p3Entity, context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, n3Entity, context);
        if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE && errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.FACE, 4, faceEntity, context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
      }
    }
    else {
      // Quads
      v1Entity[3] = "_cv4";
      if (context.warpConeEnds) {
        // Hack for mgfEntitySphere and mgfEntityTorus
        d = globalThis.Math.atan(normalOffset1) + (globalThis.Math.PI / 4) / context.numberOfQuarterCircleDivisions;
        if (d >= globalThis.Math.PI / 2 - Numeric.EPSILON) {
          normalOffset1 = Numeric.HUGE_FLOAT_VALUE;
        }
        else {
          normalOffset1 = globalThis.Math.tan(
            globalThis.Math.atan(normalOffset1) + (globalThis.Math.PI / 4) / context.numberOfQuarterCircleDivisions,
          );
        }
      }

      MgfTessellationMath.formatFloat(p4x, 24, v1Context.p.x + radius1 * u.x);
      if (normalOffset1 >= Numeric.HUGE_FLOAT_VALUE) {
        MgfTessellationMath.formatFloat(n4x, 24, w.x);
      }
      else {
        MgfTessellationMath.formatFloat(n4x, 24, u.x + w.x * normalOffset1);
      }

      MgfTessellationMath.formatFloat(p4y, 24, v1Context.p.y + radius1 * u.y);
      if (normalOffset1 >= Numeric.HUGE_FLOAT_VALUE) {
        MgfTessellationMath.formatFloat(n4y, 24, w.y);
      }
      else {
        MgfTessellationMath.formatFloat(n4y, 24, u.y + w.y * normalOffset1);
      }

      MgfTessellationMath.formatFloat(p4z, 24, v1Context.p.z + radius1 * u.z);
      if (normalOffset1 >= Numeric.HUGE_FLOAT_VALUE) {
        MgfTessellationMath.formatFloat(n4z, 24, w.z);
      }
      else {
        MgfTessellationMath.formatFloat(n4z, 24, u.z + w.z * normalOffset1);
      }
      p4Entity[1] = p4x[0];
      p4Entity[2] = p4y[0];
      p4Entity[3] = p4z[0];
      n4Entity[1] = n4x[0];
      n4Entity[2] = n4y[0];
      n4Entity[3] = n4z[0];

      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 3, v4Entity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p4Entity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, n4Entity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      for (let i = 1; i <= 4 * context.numberOfQuarterCircleDivisions; i++) {
        const theta = sign * i * (globalThis.Math.PI / 2) / context.numberOfQuarterCircleDivisions;
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v1Entity, context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v2Entity, context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }

        d = u.x * globalThis.Math.cos(theta) + v.x * globalThis.Math.sin(theta);
        MgfTessellationMath.formatFloat(p3x, 24, v2Context.p.x + radius2 * d);
        if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE) {
          MgfTessellationMath.formatFloat(n3x, 24, d + w.x * normalOffset2);
        }
        MgfTessellationMath.formatFloat(p4x, 24, v1Context.p.x + radius1 * d);
        if (normalOffset1 < Numeric.HUGE_FLOAT_VALUE) {
          MgfTessellationMath.formatFloat(n4x, 24, d + w.x * normalOffset1);
        }

        d = u.y * globalThis.Math.cos(theta) + v.y * globalThis.Math.sin(theta);
        MgfTessellationMath.formatFloat(p3y, 24, v2Context.p.y + radius2 * d);
        if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE) {
          MgfTessellationMath.formatFloat(n3y, 24, d + w.y * normalOffset2);
        }
        MgfTessellationMath.formatFloat(p4y, 24, v1Context.p.y + radius1 * d);
        if (normalOffset1 < Numeric.HUGE_FLOAT_VALUE) {
          MgfTessellationMath.formatFloat(n4y, 24, d + w.y * normalOffset1);
        }

        d = u.z * globalThis.Math.cos(theta) + v.z * globalThis.Math.sin(theta);
        MgfTessellationMath.formatFloat(p3z, 24, v2Context.p.z + radius2 * d);
        if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE) {
          MgfTessellationMath.formatFloat(n3z, 24, d + w.z * normalOffset2);
        }
        MgfTessellationMath.formatFloat(p4z, 24, v1Context.p.z + radius1 * d);
        if (normalOffset1 < Numeric.HUGE_FLOAT_VALUE) {
          MgfTessellationMath.formatFloat(n4z, 24, d + w.z * normalOffset1);
        }

        p3Entity[1] = p3x[0];
        p3Entity[2] = p3y[0];
        p3Entity[3] = p3z[0];
        n3Entity[1] = n3x[0];
        n3Entity[2] = n3y[0];
        n3Entity[3] = n3z[0];
        p4Entity[1] = p4x[0];
        p4Entity[2] = p4y[0];
        p4Entity[3] = p4z[0];
        n4Entity[1] = n4x[0];
        n4Entity[2] = n4y[0];
        n4Entity[3] = n4z[0];

        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v3Entity, context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p3Entity, context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, n3Entity, context);
        if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE && errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v4Entity, context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p4Entity, context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, n4Entity, context);
        if (normalOffset1 < Numeric.HUGE_FLOAT_VALUE && errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.FACE, 5, faceEntity, context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
      }
    }
    return ParseErrorContext.MGF_OK;
  }
}

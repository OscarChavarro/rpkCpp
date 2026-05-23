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

export class MgfRingEntityTessellator {
  private constructor() {
  }

  /**
  Turn a ring into polygons
  */
  public static handleEntity(argumentCount: number, argumentValues: string[], context: ParseRuntimeContext): number {
    const p3x = [""];
    const p3y = [""];
    const p3z = [""];
    const p4x = [""];
    const p4y = [""];
    const p4z = [""];
    const namesEntity = [
      context.entityNames[EntityTypeContext.MGF_NORMAL],
      "0",
      "0",
      "0",
    ];
    const v1Entity = [
      context.entityNames[EntityTypeContext.VERTEX],
      "_rv1",
      "=",
      "",
    ];
    const v2Entity = [
      context.entityNames[EntityTypeContext.VERTEX],
      "_rv2",
      "=",
      "_rv3",
    ];
    const v3Entity = [
      context.entityNames[EntityTypeContext.VERTEX],
      "_rv3",
      "=",
    ];
    const p3Entity = [
      context.entityNames[EntityTypeContext.MGF_POINT],
      "",
      "",
      "",
    ];
    const v4Entity = [
      context.entityNames[EntityTypeContext.VERTEX],
      "_rv4",
      "=",
    ];
    const p4Entity = [
      context.entityNames[EntityTypeContext.MGF_POINT],
      "",
      "",
      "",
    ];
    const faceEntity = [
      context.entityNames[EntityTypeContext.FACE],
      "_rv1",
      "_rv2",
      "_rv3",
      "_rv4",
    ];

    if (argumentCount !== 4) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    const vertexContext = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1]!, context) as VertexContext | null;
    if (vertexContext === null) {
      return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if (vertexContext.n.isNull(Numeric.EPSILON)) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if (!TokenValidationContext.isFloat(argumentValues[2]!) || !TokenValidationContext.isFloat(argumentValues[3]!)) {
      return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
    }
    let minRadius = Number.parseFloat(argumentValues[2]!);
    if (minRadius <= Numeric.EPSILON && minRadius >= -Numeric.EPSILON) {
      minRadius = 0.0;
    }
    const maxRadius = Number.parseFloat(argumentValues[3]!);
    if (minRadius < 0.0 || maxRadius <= minRadius) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Initialize
    const u = new Vector3Dd();
    const v = new Vector3Dd();
    MgfTessellationMath.mgfMakeAxes(u, v, vertexContext.n, Numeric.EPSILON);
    MgfTessellationMath.formatFloat(p3x, 24, vertexContext.p.x + maxRadius * u.x);
    MgfTessellationMath.formatFloat(p3y, 24, vertexContext.p.y + maxRadius * u.y);
    MgfTessellationMath.formatFloat(p3z, 24, vertexContext.p.z + maxRadius * u.z);
    p3Entity[1] = p3x[0]!;
    p3Entity[2] = p3y[0]!;
    p3Entity[3] = p3z[0]!;
    let errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 3, v3Entity as string[], context);
    if (errorCode !== ParseErrorContext.MGF_OK) {
      return errorCode;
    }
    errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p3Entity as string[], context);
    if (errorCode !== ParseErrorContext.MGF_OK) {
      return errorCode;
    }

    if (Numeric.doubleEqual(minRadius, 0.0, Numeric.EPSILON)) {
      // Closed
      v1Entity[3] = argumentValues[1]!;
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v1Entity as string[], context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, namesEntity as string[], context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      for (let i = 1; i <= 4 * context.numberOfQuarterCircleDivisions; i++) {
        const theta = i * (globalThis.Math.PI / 2) / context.numberOfQuarterCircleDivisions;
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v2Entity as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }

        MgfTessellationMath.formatFloat(
          p3x,
          24,
          vertexContext.p.x + maxRadius * u.x * globalThis.Math.cos(theta) + maxRadius * v.x * globalThis.Math.sin(theta),
        );
        MgfTessellationMath.formatFloat(
          p3y,
          24,
          vertexContext.p.y + maxRadius * u.y * globalThis.Math.cos(theta) + maxRadius * v.y * globalThis.Math.sin(theta),
        );
        MgfTessellationMath.formatFloat(
          p3z,
          24,
          vertexContext.p.z + maxRadius * u.z * globalThis.Math.cos(theta) + maxRadius * v.z * globalThis.Math.sin(theta),
        );

        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v3Entity as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        p3Entity[1] = p3x[0]!;
        p3Entity[2] = p3y[0]!;
        p3Entity[3] = p3z[0]!;
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p3Entity as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.FACE, 4, faceEntity as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
      }
    }
    else {
      // Open
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 3, v4Entity as string[], context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }

      MgfTessellationMath.formatFloat(p4x, 24, vertexContext.p.x + minRadius * u.x);
      MgfTessellationMath.formatFloat(p4y, 24, vertexContext.p.y + minRadius * u.y);
      MgfTessellationMath.formatFloat(p4z, 24, vertexContext.p.z + minRadius * u.z);
      p4Entity[1] = p4x[0]!;
      p4Entity[2] = p4y[0]!;
      p4Entity[3] = p4z[0]!;

      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p4Entity as string[], context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      v1Entity[3] = "_rv4";
      for (let i = 1; i <= 4 * context.numberOfQuarterCircleDivisions; i++) {
        const theta = i * (globalThis.Math.PI / 2) / context.numberOfQuarterCircleDivisions;
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v1Entity as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v2Entity as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }

        let delta = u.x * globalThis.Math.cos(theta) + v.x * globalThis.Math.sin(theta);
        MgfTessellationMath.formatFloat(p3x, 24, vertexContext.p.x + maxRadius * delta);
        MgfTessellationMath.formatFloat(p4x, 24, vertexContext.p.x + minRadius * delta);

        delta = u.y * globalThis.Math.cos(theta) + v.y * globalThis.Math.sin(theta);
        MgfTessellationMath.formatFloat(p3y, 24, vertexContext.p.y + maxRadius * delta);
        MgfTessellationMath.formatFloat(p4y, 24, vertexContext.p.y + minRadius * delta);

        delta = u.z * globalThis.Math.cos(theta) + v.z * globalThis.Math.sin(theta);
        MgfTessellationMath.formatFloat(p3z, 24, vertexContext.p.z + maxRadius * delta);
        MgfTessellationMath.formatFloat(p4z, 24, vertexContext.p.z + minRadius * delta);

        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v3Entity as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        p3Entity[1] = p3x[0]!;
        p3Entity[2] = p3y[0]!;
        p3Entity[3] = p3z[0]!;
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p3Entity as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v4Entity as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        p4Entity[1] = p4x[0]!;
        p4Entity[2] = p4y[0]!;
        p4Entity[3] = p4z[0]!;
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p4Entity as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.FACE, 5, faceEntity as string[], context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
      }
    }
    return ParseErrorContext.MGF_OK;
  }
}

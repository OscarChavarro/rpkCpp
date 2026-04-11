import { Numeric } from "../../common/linealAlgebra/Numeric";
import { EntityTypeContext } from "../context/EntityTypeContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { TokenValidationContext } from "../context/TokenValidationContext";
import { VertexContext } from "../context/VertexContext";
import { MgfEntityControl } from "./MgfEntityControl";
import { MgfTessellationMath } from "./MgfTessellationMath";
import { MgfVertexFaceEntitySupport } from "./MgfVertexFaceEntitySupport";

export class MgfTorusEntityExpander {
  private constructor() {
  }

  /**
  Expand a torus into cones
  */
  public static handleEntity(argumentCount: number, argumentValues: string[], context: ParseRuntimeContext): number {
    const p2x = [""];
    const p2y = [""];
    const p2z = [""];
    const radius1 = [""];
    const radius2 = [""];
    const v1Entity = [
      context.entityNames[EntityTypeContext.VERTEX],
      "_tv1",
      "=",
      "_tv2",
    ];
    const v2Entity = [
      context.entityNames[EntityTypeContext.VERTEX],
      "_tv2",
      "=",
      "",
    ];
    const p2Entity = [
      context.entityNames[EntityTypeContext.MGF_POINT],
      "",
      "",
      "",
    ];
    const coneEntity = [
      context.entityNames[EntityTypeContext.CONE],
      "_tv1",
      "",
      "_tv2",
      "",
    ];
    let vertexContext: VertexContext | null;
    let averageRadius: number;

    if (argumentCount !== 4) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    vertexContext = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1], context);
    if (vertexContext === null) {
      return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if (vertexContext.n.isNull(Numeric.EPSILON)) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if (!TokenValidationContext.isFloat(argumentValues[2]) || !TokenValidationContext.isFloat(argumentValues[3])) {
      return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
    }
    let minRadius = Number.parseFloat(argumentValues[2]);
    if (minRadius <= Numeric.EPSILON && minRadius >= -Numeric.EPSILON) {
      minRadius = 0.0;
    }
    const maxRadius = Number.parseFloat(argumentValues[3]);

    // Check orientation
    let sign: number;
    if (minRadius > 0.0) {
      sign = 1;
    }
    else if (minRadius < 0.0) {
      sign = -1;
    }
    else {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if (sign * (maxRadius - minRadius) <= 0.0) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Initialize
    context.warpConeEnds = true;
    context.geometryBuildState.warpConeEnds = true;
    v2Entity[3] = argumentValues[1];
    MgfTessellationMath.formatFloat(
      p2x,
      24,
      vertexContext.p.x + 0.5 * sign * (maxRadius - minRadius) * vertexContext.n.x,
    );
    MgfTessellationMath.formatFloat(
      p2y,
      24,
      vertexContext.p.y + 0.5 * sign * (maxRadius - minRadius) * vertexContext.n.y,
    );
    MgfTessellationMath.formatFloat(
      p2z,
      24,
      vertexContext.p.z + 0.5 * sign * (maxRadius - minRadius) * vertexContext.n.z,
    );
    p2Entity[1] = p2x[0];
    p2Entity[2] = p2y[0];
    p2Entity[3] = p2z[0];
    let errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v2Entity, context);
    if (errorCode !== ParseErrorContext.MGF_OK) {
      return errorCode;
    }
    errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p2Entity, context);
    if (errorCode !== ParseErrorContext.MGF_OK) {
      return errorCode;
    }
    averageRadius = 0.5 * (minRadius + maxRadius);
    MgfTessellationMath.formatFloat(radius2, 24, averageRadius);

    // Run outer section
    let i = 1;
    for (; i <= 2 * context.numberOfQuarterCircleDivisions; i++) {
      const theta = i * (globalThis.Math.PI / 2) / context.numberOfQuarterCircleDivisions;
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v1Entity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      MgfTessellationMath.formatFloat(
        p2x,
        24,
        vertexContext.p.x + 0.5 * sign * (maxRadius - minRadius) * globalThis.Math.cos(theta) * vertexContext.n.x,
      );
      MgfTessellationMath.formatFloat(
        p2y,
        24,
        vertexContext.p.y + 0.5 * sign * (maxRadius - minRadius) * globalThis.Math.cos(theta) * vertexContext.n.y,
      );
      MgfTessellationMath.formatFloat(
        p2z,
        24,
        vertexContext.p.z + 0.5 * sign * (maxRadius - minRadius) * globalThis.Math.cos(theta) * vertexContext.n.z,
      );
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v2Entity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      p2Entity[1] = p2x[0];
      p2Entity[2] = p2y[0];
      p2Entity[3] = p2z[0];
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p2Entity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      radius1[0] = radius2[0];
      MgfTessellationMath.formatFloat(radius2, 24, averageRadius + 0.5 * (maxRadius - minRadius) * globalThis.Math.sin(theta));
      coneEntity[2] = radius1[0];
      coneEntity[4] = radius2[0];
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.CONE, 5, coneEntity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
    }

    // Run inner section
    MgfTessellationMath.formatFloat(radius2, 24, -0.5 * (minRadius + maxRadius));
    for (; i <= 4 * context.numberOfQuarterCircleDivisions; i++) {
      const theta = i * (globalThis.Math.PI / 2) / context.numberOfQuarterCircleDivisions;
      MgfTessellationMath.formatFloat(
        p2x,
        24,
        vertexContext.p.x + 0.5 * sign * (maxRadius - minRadius) * globalThis.Math.cos(theta) * vertexContext.n.x,
      );
      MgfTessellationMath.formatFloat(
        p2y,
        24,
        vertexContext.p.y + 0.5 * sign * (maxRadius - minRadius) * globalThis.Math.cos(theta) * vertexContext.n.y,
      );
      MgfTessellationMath.formatFloat(
        p2z,
        24,
        vertexContext.p.z + 0.5 * sign * (maxRadius - minRadius) * globalThis.Math.cos(theta) * vertexContext.n.z,
      );
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v1Entity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v2Entity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      p2Entity[1] = p2x[0];
      p2Entity[2] = p2y[0];
      p2Entity[3] = p2z[0];
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p2Entity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      radius1[0] = radius2[0];
      MgfTessellationMath.formatFloat(
        radius2,
        24,
        -averageRadius - 0.5 * (maxRadius - minRadius) * globalThis.Math.sin(theta),
      );
      coneEntity[2] = radius1[0];
      coneEntity[4] = radius2[0];
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.CONE, 5, coneEntity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
    }
    context.warpConeEnds = false;
    context.geometryBuildState.warpConeEnds = false;
    return ParseErrorContext.MGF_OK;
  }
}

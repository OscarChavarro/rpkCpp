import { EntityTypeContext } from "../context/EntityTypeContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { TokenValidationContext } from "../context/TokenValidationContext";
import { VertexContext } from "../context/VertexContext";
import { MgfEntityControl } from "./MgfEntityControl";
import { MgfTessellationMath } from "./MgfTessellationMath";
import { MgfVertexFaceEntitySupport } from "./MgfVertexFaceEntitySupport";

export class MgfSphereEntityExpander {
  private constructor() {
  }

  /**
  Expand a sphere into cones
  */
  public static handleEntity(argumentCount: number, argumentValues: string[], context: ParseRuntimeContext): number {
    const p2x = [""];
    const p2y = [""];
    const p2z = [""];
    const radius1 = [""];
    const radius2 = [""];

    const v1Entity = [
      context.entityNames[EntityTypeContext.VERTEX],
      "_sv1",
      "=",
      "_sv2",
    ];
    const v2Entity = [
      context.entityNames[EntityTypeContext.VERTEX],
      "_sv2",
      "=",
    ];
    const p2Entity = new Array<string>(5);
    p2Entity[0] = context.entityNames[EntityTypeContext.MGF_POINT]!;
    const coneEntity = new Array<string>(6);
    coneEntity[0] = context.entityNames[EntityTypeContext.CONE]!;
    coneEntity[1] = "_sv1";
    coneEntity[3] = "_sv2";

    if (argumentCount !== 3) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    const vertexContext = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1]!, context) as VertexContext | null;
    if (vertexContext === null) {
      return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if (!TokenValidationContext.isFloat(argumentValues[2]!)) {
      return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
    }
    const radius = Number.parseFloat(argumentValues[2]!);

    // Initialize
    context.warpConeEnds = true;
    context.geometryBuildState.warpConeEnds = true;
    let errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 3, v2Entity as string[], context);
    if (errorCode !== ParseErrorContext.MGF_OK) {
      return errorCode;
    }
    MgfTessellationMath.formatFloat(p2x, 24, vertexContext.p.x);
    MgfTessellationMath.formatFloat(p2y, 24, vertexContext.p.y);
    MgfTessellationMath.formatFloat(p2z, 24, vertexContext.p.z + radius);
    p2Entity[1] = p2x[0]!;
    p2Entity[2] = p2y[0]!;
    p2Entity[3] = p2z[0]!;
    errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p2Entity as string[], context);
    if (errorCode !== ParseErrorContext.MGF_OK) {
      return errorCode;
    }
    radius2[0] = "0";

    for (let i = 1; i <= 2 * context.numberOfQuarterCircleDivisions; i++) {
      const theta = i * (globalThis.Math.PI / 2) / context.numberOfQuarterCircleDivisions;
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v1Entity as string[], context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      MgfTessellationMath.formatFloat(p2z, 24, vertexContext.p.z + radius * globalThis.Math.cos(theta));
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v2Entity as string[], context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      p2Entity[3] = p2z[0]!;
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p2Entity as string[], context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
      radius1[0] = radius2[0]!;
      MgfTessellationMath.formatFloat(radius2, 24, radius * globalThis.Math.sin(theta));
      coneEntity[2] = radius1[0]!;
      coneEntity[4] = radius2[0]!;
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.CONE, 5, coneEntity as string[], context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
    }
    context.warpConeEnds = false;
    context.geometryBuildState.warpConeEnds = false;
    return ParseErrorContext.MGF_OK;
  }
}

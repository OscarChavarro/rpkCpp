import { Numeric } from "../../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../../common/linealAlgebra/Vector3D";
import { GalerkinElement } from "../../GalerkinElement";
import { GalerkinState } from "../../GalerkinState";
import { SglContext } from "../../../render/sgl/SglContext";
import { Patch } from "../../../skin/Patch";
import { ClusterLeafVisitor } from "./ClusterLeafVisitor";

export class ScratchRendererVisitor implements ClusterLeafVisitor {
  private readonly eyePoint: Vector3D;
  private readonly sglContext: SglContext | null;

  public constructor(inEyePoint: Vector3D, inSglContext: SglContext | null) {
    this.eyePoint = inEyePoint;
    this.sglContext = inSglContext;
  }

  public visit(
    galerkinElement: GalerkinElement,
    _galerkinState: GalerkinState,
  ): void {
    const patch = galerkinElement.patch;
    if (patch === null) {
      return;
    }

    if (patch.normal.dotProduct(this.eyePoint) + patch.planeConstant < Numeric.EPSILON) {
      return;
    }

    const v = new Array<Vector3D>(4);
    for (let i = 0; i < patch.numberOfVertices; i++) {
      if (patch.vertex[i] !== null && patch.vertex[i]!.point !== null) {
        v[i] = new Vector3D(patch.vertex[i]!.point.x, patch.vertex[i]!.point.y, patch.vertex[i]!.point.z);
      }
      else {
        v[i] = new Vector3D();
      }
    }

    if (this.sglContext === null) {
      return;
    }

    this.sglContext.sglSetGalerkinElement(galerkinElement);
    this.sglContext.sglPolygon((patch as Patch).numberOfVertices, v);
  }
}

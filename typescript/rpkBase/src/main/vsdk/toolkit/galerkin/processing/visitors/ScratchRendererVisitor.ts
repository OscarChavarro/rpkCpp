import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { GalerkinElement } from "../../GalerkinElement";
import { GalerkinState } from "../../GalerkinState";
import { SglContext } from "../../../render/sgl/SglContext";
import { Patch } from "../../../environment/geometry/elements/Patch";
import { ClusterLeafVisitor } from "./ClusterLeafVisitor";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class ScratchRendererVisitor implements ClusterLeafVisitor {
  private readonly eyePoint: Vector3Df;
  private readonly sglContext: SglContext | null;

  public constructor(inEyePoint: Vector3Df, inSglContext: SglContext | null) {
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

    if (patch.getNormal().dotProduct(this.eyePoint) + patch.getPlaneConstant() < VSDK.EPSILON) {
      return;
    }

    const v = new Array<Vector3Df>(4);
    for (let i = 0; i < patch.numberOfVertices; i++) {
      if (patch.vertex[i] !== null && patch.vertex[i]!.point !== null) {
        v[i] = new Vector3Df(patch.vertex[i]!.point.x(), patch.vertex[i]!.point.y(), patch.vertex[i]!.point.z());
      }
      else {
        v[i] = new Vector3Df();
      }
    }

    if (this.sglContext === null) {
      return;
    }

    this.sglContext.sglSetGalerkinElement(galerkinElement);
    this.sglContext.sglPolygon((patch as Patch).numberOfVertices, v);
  }
}

import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { GalerkinElement } from "../../GalerkinElement";
import { GalerkinState } from "../../GalerkinState";
import type { ScratchVisibilityBuffer } from "../ScratchVisibilityWorkspace";
import { ClusterLeafVisitor } from "./ClusterLeafVisitor";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class ScratchRendererVisitor implements ClusterLeafVisitor {
  private readonly eyePoint: Vector3Dd;
  private readonly target: ScratchVisibilityBuffer;
  // TypeScript: the vertices of the element drawn, reused from one visit to the next
  private readonly v: Vector3Dd[];

  public constructor(inEyePoint: Vector3Dd, inTarget: ScratchVisibilityBuffer) {
    this.eyePoint = inEyePoint;
    this.target = inTarget;
    this.v = new Array<Vector3Dd>(4);
  }

  public visit(
    galerkinElement: GalerkinElement,
    _galerkinState: GalerkinState,
  ): void {
    const patch = galerkinElement.patch!;

    // Backface culling test: only render the element if it is turned towards
    // the current eye point
    if (patch.getNormal().dotProduct(this.eyePoint) + patch.getPlaneConstant() < VSDK.EPSILON) {
      return;
    }

    for (let i = 0; i < patch.numberOfVertices; i++) {
      this.v[i] = patch.vertex[i]!.point;
    }

    this.target.drawPolygon(patch.numberOfVertices, this.v, galerkinElement);
  }
}

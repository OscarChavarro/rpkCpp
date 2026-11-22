import { ColorRgbMutable } from "../../../common/color/ColorRgbMutable";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { GalerkinElement } from "../../GalerkinElement";
import { GalerkinIterationMethod } from "../../GalerkinIterationMethod";
import { GalerkinState } from "../../GalerkinState";
import { ClusterLeafVisitor } from "./ClusterLeafVisitor";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class PowerAccumulatorVisitor implements ClusterLeafVisitor {
  private readonly sourceRadiance: ColorRgbMutable;
  private readonly samplePoint: Vector3Df;
  private readonly accumulatedRadiance: ColorRgbMutable;

  public constructor(inSourceRadiance: ColorRgbMutable, inSamplePoint: Vector3Df) {
    this.sourceRadiance = inSourceRadiance;
    this.samplePoint = inSamplePoint;
    this.accumulatedRadiance = new ColorRgbMutable();
    this.accumulatedRadiance.clear();
  }

  public getAccumulatedRadiance(): ColorRgbMutable {
    return this.accumulatedRadiance;
  }

  public visit(
    galerkinElement: GalerkinElement,
    galerkinState: GalerkinState,
  ): void {
    let dir = new Vector3Df();
    dir = this.samplePoint.subtract((galerkinElement.patch as NonNullable<GalerkinElement["patch"]>).midPoint());
    const dist = dir.length();
    let srcOs: number;
    if (dist < VSDK.EPSILON) {
      srcOs = 1.0;
    }
    else {
      srcOs = dir.dotProduct((galerkinElement.patch as NonNullable<GalerkinElement["patch"]>).getNormal()) / dist;
    }
    if (srcOs <= 0.0) {
      return;
    }

    let rad: ColorRgbMutable;
    if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL
      || galerkinState.isJacobi()) {
      rad = (galerkinElement.radiance as ColorRgbMutable[])[0]!;
    }
    else {
      rad = (galerkinElement.unShotRadiance as ColorRgbMutable[])[0]!;
    }

    this.accumulatedRadiance.addScaled(this.sourceRadiance, srcOs * galerkinElement.area, rad);
  }
}

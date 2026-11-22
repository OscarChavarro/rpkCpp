import { ColorRgbMutable } from "../../../common/color/ColorRgbMutable";
import { Numeric } from "../../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../../common/linealAlgebra/Vector3D";
import { GalerkinElement } from "../../GalerkinElement";
import { GalerkinIterationMethod } from "../../GalerkinIterationMethod";
import { GalerkinState } from "../../GalerkinState";
import { ClusterLeafVisitor } from "./ClusterLeafVisitor";

export class PowerAccumulatorVisitor implements ClusterLeafVisitor {
  private readonly sourceRadiance: ColorRgbMutable;
  private readonly samplePoint: Vector3D;
  private readonly accumulatedRadiance: ColorRgbMutable;

  public constructor(inSourceRadiance: ColorRgbMutable, inSamplePoint: Vector3D) {
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
    const dir = new Vector3D();
    dir.subtraction(this.samplePoint, (galerkinElement.patch as NonNullable<GalerkinElement["patch"]>).midPoint());
    const dist = dir.norm();
    let srcOs: number;
    if (dist < Numeric.EPSILON) {
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

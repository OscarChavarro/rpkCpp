import { ColorRgb } from "../../../common/color/ColorRgb";
import { Numeric } from "../../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../../common/linealAlgebra/Vector3D";
import { GalerkinElement } from "../../GalerkinElement";
import { GalerkinIterationMethod } from "../../GalerkinIterationMethod";
import { GalerkinState } from "../../GalerkinState";
import { ClusterLeafVisitor } from "./ClusterLeafVisitor";

export class PowerAccumulatorVisitor implements ClusterLeafVisitor {
  private readonly sourceRadiance: ColorRgb;
  private readonly samplePoint: Vector3D;
  private readonly accumulatedRadiance: ColorRgb;

  public constructor(inSourceRadiance: ColorRgb, inSamplePoint: Vector3D) {
    this.sourceRadiance = inSourceRadiance;
    this.samplePoint = inSamplePoint;
    this.accumulatedRadiance = new ColorRgb();
    this.accumulatedRadiance.clear();
  }

  public getAccumulatedRadiance(): ColorRgb {
    return this.accumulatedRadiance;
  }

  public visit(
    galerkinElement: GalerkinElement,
    galerkinState: GalerkinState,
  ): void {
    const dir = new Vector3D();
    dir.subtraction(this.samplePoint, (galerkinElement.patch as NonNullable<GalerkinElement["patch"]>).midPoint);
    const dist = dir.norm();
    let srcOs: number;
    if (dist < Numeric.EPSILON) {
      srcOs = 1.0;
    }
    else {
      srcOs = dir.dotProduct((galerkinElement.patch as NonNullable<GalerkinElement["patch"]>).normal) / dist;
    }
    if (srcOs <= 0.0) {
      return;
    }

    let rad: ColorRgb;
    if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL
      || galerkinState.galerkinIterationMethod === GalerkinIterationMethod.JACOBI) {
      rad = (galerkinElement.radiance as ColorRgb[])[0]!;
    }
    else {
      rad = (galerkinElement.unShotRadiance as ColorRgb[])[0]!;
    }

    this.accumulatedRadiance.addScaled(this.sourceRadiance, srcOs * galerkinElement.area, rad);
  }
}

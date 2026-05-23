import { ColorRgb } from "../../../common/color/ColorRgb";
import { GalerkinElement } from "../../GalerkinElement";
import { GalerkinIterationMethod } from "../../GalerkinIterationMethod";
import { GalerkinState } from "../../GalerkinState";
import { ClusterLeafVisitor } from "./ClusterLeafVisitor";

export class MaximumRadianceVisitor implements ClusterLeafVisitor {
  private accumulatedRadiance: ColorRgb;

  public constructor() {
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
    let rad: ColorRgb;
    if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL
      || galerkinState.galerkinIterationMethod === GalerkinIterationMethod.JACOBI) {
      rad = (galerkinElement.radiance as ColorRgb[])[0]!;
    }
    else {
      rad = (galerkinElement.unShotRadiance as ColorRgb[])[0]!;
    }
    this.accumulatedRadiance.maximum(this.accumulatedRadiance, rad);
  }
}

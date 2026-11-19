import { ColorRgbMutable } from "../../../common/color/ColorRgbMutable";
import { GalerkinElement } from "../../GalerkinElement";
import { GalerkinIterationMethod } from "../../GalerkinIterationMethod";
import { GalerkinState } from "../../GalerkinState";
import { ClusterLeafVisitor } from "./ClusterLeafVisitor";

export class MaximumRadianceVisitor implements ClusterLeafVisitor {
  private accumulatedRadiance: ColorRgbMutable;

  public constructor() {
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
    let rad: ColorRgbMutable;
    if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL
      || galerkinState.galerkinIterationMethod === GalerkinIterationMethod.JACOBI) {
      rad = (galerkinElement.radiance as ColorRgbMutable[])[0]!;
    }
    else {
      rad = (galerkinElement.unShotRadiance as ColorRgbMutable[])[0]!;
    }
    this.accumulatedRadiance.maximum(this.accumulatedRadiance, rad);
  }
}

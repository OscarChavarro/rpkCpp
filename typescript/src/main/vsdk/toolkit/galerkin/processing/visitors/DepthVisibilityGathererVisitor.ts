import { ColorRgb } from "../../../common/ColorRgb";
import { GalerkinElement } from "../../GalerkinElement";
import { GalerkinState } from "../../GalerkinState";
import { Interaction } from "../../Interaction";
import { ClusterTraversalStrategy } from "../ClusterTraversalStrategy";
import { ClusterLeafVisitor } from "./ClusterLeafVisitor";

export class DepthVisibilityGathererVisitor implements ClusterLeafVisitor {
  private readonly link: Interaction;
  private readonly sourceRadiance: ColorRgb[];
  private readonly pixelArea: number;

  public constructor(
    inLink: Interaction,
    inSourceRadiance: ColorRgb[],
    inPixelArea: number,
  ) {
    this.link = inLink;
    this.sourceRadiance = inSourceRadiance;
    this.pixelArea = inPixelArea;
  }

  public visit(
    galerkinElement: GalerkinElement,
    _galerkinState: GalerkinState,
  ): void {
    if (galerkinElement.scratchVisibilityUsageCounter <= 0) {
      return;
    }

    const areaFactor = this.pixelArea * galerkinElement.scratchVisibilityUsageCounter / (0.25 * this.link.receiverElement.area);
    ClusterTraversalStrategy.isotropicGatherRadiance(galerkinElement, areaFactor, this.link, this.sourceRadiance);

    galerkinElement.scratchVisibilityUsageCounter = 0;
  }
}

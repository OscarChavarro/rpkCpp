import { ColorRgb } from "../../../common/color/ColorRgb";
import { GalerkinElement } from "../../GalerkinElement";
import { GalerkinState } from "../../GalerkinState";
import { Interaction } from "../../Interaction";
import { ClusterTraversalStrategy } from "../ClusterTraversalStrategy";
import { ClusterLeafVisitor } from "./ClusterLeafVisitor";

export class OrientedGathererVisitor implements ClusterLeafVisitor {
  private readonly link: Interaction;
  private readonly sourceRadiance: ColorRgb[];

  public constructor(inLink: Interaction, inSourceRadiance: ColorRgb[]) {
    this.link = inLink;
    this.sourceRadiance = inSourceRadiance;
  }

  public visit(
    galerkinElement: GalerkinElement,
    _galerkinState: GalerkinState,
  ): void {
    const areaFactor = ClusterTraversalStrategy.surfaceProjectedAreaToSamplePoint(galerkinElement)
      / (0.25 * this.link.receiverElement.area);

    ClusterTraversalStrategy.isotropicGatherRadiance(galerkinElement, areaFactor, this.link, this.sourceRadiance);
  }
}

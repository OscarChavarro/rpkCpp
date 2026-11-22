import { ColorRgbMutable } from "../../../common/color/ColorRgbMutable";
import { GalerkinElement } from "../../GalerkinElement";
import { GalerkinState } from "../../GalerkinState";
import { Interaction } from "../../Interaction";
import { ClusterTraversalStrategy } from "../ClusterTraversalStrategy";
import { ClusterLeafVisitor } from "./ClusterLeafVisitor";

export class OrientedGathererVisitor implements ClusterLeafVisitor {
  private readonly link: Interaction;
  private readonly sourceRadiance: ColorRgbMutable[];

  public constructor(inLink: Interaction, inSourceRadiance: ColorRgbMutable[]) {
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

import { GalerkinElement } from "../../GalerkinElement";
import { GalerkinState } from "../../GalerkinState";
import { ClusterTraversalStrategy } from "../ClusterTraversalStrategy";
import { ClusterLeafVisitor } from "./ClusterLeafVisitor";

export class ProjectedAreaAccumulatorVisitor implements ClusterLeafVisitor {
  private totalProjectedArea: number;

  public constructor() {
    this.totalProjectedArea = 0.0;
  }

  public visit(
    galerkinElement: GalerkinElement,
    _galerkinState: GalerkinState,
  ): void {
    this.totalProjectedArea += ClusterTraversalStrategy.surfaceProjectedAreaToSamplePoint(galerkinElement);
  }

  public getTotalProjectedArea(): number {
    return this.totalProjectedArea;
  }
}

import { GalerkinElement } from "../../GalerkinElement";
import { GalerkinState } from "../../GalerkinState";

export interface ClusterLeafVisitor {
  visit(galerkinElement: GalerkinElement, galerkinState: GalerkinState): void;
}

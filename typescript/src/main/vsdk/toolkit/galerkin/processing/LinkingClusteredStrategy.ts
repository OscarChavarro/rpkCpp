import { Numeric } from "../../common/linealAlgebra/Numeric";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinIterationMethod } from "../GalerkinIterationMethod";
import { GalerkinRole } from "../GalerkinRole";
import { GalerkinState } from "../GalerkinState";
import { Interaction } from "../Interaction";

export class LinkingClusteredStrategy {
  public static createInitialLinks(
    element: GalerkinElement,
    role: GalerkinRole,
    galerkinState: GalerkinState,
  ): void {
    let receiverElement: GalerkinElement | null;
    let sourceElement: GalerkinElement | null;

    switch (role) {
      case GalerkinRole.RECEIVER:
        receiverElement = element;
        sourceElement = galerkinState.topCluster;
        break;
      case GalerkinRole.SOURCE:
        sourceElement = element;
        receiverElement = galerkinState.topCluster;
        break;
      default:
        return;
    }

    if (receiverElement === null || sourceElement === null) {
      return;
    }

    let n = receiverElement.basisSize * sourceElement.basisSize;
    if (n <= 0) {
      n = 1;
    }
    const K = new Array<number>(n).fill(0.0);

    const deltaK = new Array<number>(1);
    deltaK[0] = Numeric.HUGE_FLOAT_VALUE;

    const newLink = new Interaction(
      receiverElement,
      sourceElement,
      K,
      deltaK,
      receiverElement.basisSize,
      sourceElement.basisSize,
      1,
      128,
    );

    if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.SOUTH_WELL) {
      if (sourceElement.interactions === null) {
        sourceElement.interactions = [];
      }
      sourceElement.interactions.push(newLink);
    }
    else {
      if (receiverElement.interactions === null) {
        receiverElement.interactions = [];
      }
      receiverElement.interactions.push(newLink);
    }
  }
}

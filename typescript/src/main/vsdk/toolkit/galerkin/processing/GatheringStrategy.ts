import { RenderOptions } from "../../common/RenderOptions";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinState } from "../GalerkinState";
import { Scene } from "../../scene/Scene";

export abstract class GatheringStrategy {
  protected static pushPullPotential(element: GalerkinElement | null, down: number): number {
    if (element === null) {
      return 0.0;
    }

    down += element.area > 0.0 ? (element.receivedPotential / element.area) : 0.0;
    element.receivedPotential = 0.0;
    let up = 0.0;

    if (element.regularSubElements === null && element.irregularSubElements === null) {
      up = down + (element.patch !== null ? element.patch.directPotential : 0.0);
    }

    if (element.regularSubElements !== null) {
      for (let i = 0; i < 4; i++) {
        if (element.regularSubElements[i] instanceof GalerkinElement) {
          up += 0.25 * GatheringStrategy.pushPullPotential(element.regularSubElements[i] as GalerkinElement, down);
        }
      }
    }

    if (element.irregularSubElements !== null) {
      for (let j = 0; j < element.irregularSubElements.length; j++) {
        if (!(element.irregularSubElements[j] instanceof GalerkinElement)) {
          continue;
        }
        const subElement = element.irregularSubElements[j] as GalerkinElement;
        const localDown = element.isCluster() ? down : 0.0;
        up += element.area > 0.0
          ? (subElement.area / element.area) * GatheringStrategy.pushPullPotential(subElement, localDown)
          : 0.0;
      }
    }

    element.potential = up;
    return up;
  }

  public constructor() {
  }

  public abstract doGatheringIteration(scene: Scene, galerkinState: GalerkinState, renderOptions: RenderOptions): boolean;
}

import { ColorRgb } from "../../common/color/ColorRgb";
import { Patch } from "../../environment/geometry/elements/Patch";
import { GalerkinBasis } from "./GalerkinBasis";
type StochasticRadiosityElement = any;

export class McradP {
  private constructor() {
  }

  public static numberOfVertices(elem: StochasticRadiosityElement): number {
    return elem.patch!.numberOfVertices;
  }

  public static topLevelStochasticRadiosityElement(patch: Patch): StochasticRadiosityElement {
    return patch.radianceData as unknown as StochasticRadiosityElement;
  }

  public static getTopLevelPatchRad(patch: Patch): ColorRgb[] | null {
    return McradP.topLevelStochasticRadiosityElement(patch).radiance;
  }

  public static getTopLevelPatchUnShotRad(patch: Patch): ColorRgb[] | null {
    return McradP.topLevelStochasticRadiosityElement(patch).unShotRadiance;
  }

  public static getTopLevelPatchReceivedRad(patch: Patch): ColorRgb[] | null {
    return McradP.topLevelStochasticRadiosityElement(patch).receivedRadiance;
  }

  public static getTopLevelPatchBasis(patch: Patch): GalerkinBasis | null {
    return McradP.topLevelStochasticRadiosityElement(patch).basis;
  }
}

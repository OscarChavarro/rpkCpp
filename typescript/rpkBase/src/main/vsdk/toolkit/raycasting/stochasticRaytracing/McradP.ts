import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";
import { Patch } from "../../environment/geometry/elements/Patch";
import { ElementHierarchyState } from "./ElementHierarchyState";
import { StochasticRadiosityBasis } from "./StochasticRadiosityBasis";
type StochasticRadiosityElement = any;

export class McradP {
  private constructor() {
  }

  public static numberOfVertices(elem: StochasticRadiosityElement): number {
    return elem.patch!.numberOfVertices;
  }

  public static topLevelStochasticRadiosityElement(patch: Patch): StochasticRadiosityElement {
    return ElementHierarchyState.activeState().getRadianceElements()!.get(patch.getFaceIndex()) as unknown as StochasticRadiosityElement;
  }

  public static getTopLevelPatchRad(patch: Patch): ColorRgbMutable[] | null {
    return McradP.topLevelStochasticRadiosityElement(patch).radiance;
  }

  public static getTopLevelPatchUnShotRad(patch: Patch): ColorRgbMutable[] | null {
    return McradP.topLevelStochasticRadiosityElement(patch).unShotRadiance;
  }

  public static getTopLevelPatchReceivedRad(patch: Patch): ColorRgbMutable[] | null {
    return McradP.topLevelStochasticRadiosityElement(patch).receivedRadiance;
  }

  public static getTopLevelPatchBasis(patch: Patch): StochasticRadiosityBasis | null {
    return McradP.topLevelStochasticRadiosityElement(patch).basis;
  }
}

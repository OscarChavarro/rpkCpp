import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Ray } from "../../common/linealAlgebra/Ray";
import { GalerkinElement } from "../GalerkinElement";
import { Interaction } from "../Interaction";
import { ShadowCache } from "../ShadowCache";
import { CubatureRule } from "../../numericalAnalysis/CubatureRule";
import { Geometry } from "../../skin/Geometry";

export class FormFactorClusteredStrategy {
  public static doConstantAreaToAreaFormFactor(
    link: Interaction,
    cubatureRuleRcv: CubatureRule,
    cubatureRuleSrc: CubatureRule,
    Gxy: number[][],
  ): void {
    const receiverElement = link.receiverElement;
    const sourceElement = link.sourceElement;

    let G = 0.0;
    let gMin = Numeric.HUGE_DOUBLE_VALUE;
    let gMax = -Numeric.HUGE_DOUBLE_VALUE;

    for (let k = 0; k < cubatureRuleRcv.numberOfNodes; k++) {
      let Gx = 0.0;
      for (let l = 0; l < cubatureRuleSrc.numberOfNodes; l++) {
        Gx += cubatureRuleSrc.w[l]! * Gxy[k]![l]!;
      }
      Gx *= sourceElement.area;

      G += cubatureRuleRcv.w[k]! * Gx;

      if (Gx > gMax) {
        gMax = Gx;
      }
      if (Gx < gMin) {
        gMin = Gx;
      }
    }

    link.K[0] = receiverElement.area * G;

    link.deltaK = G - gMin;
    if (gMax - G > link.deltaK) {
      link.deltaK = gMax - G;
    }
    link.numberOfReceiverCubaturePositions = 1;
  }

  public static geomListMultiResolutionVisibility(
    _geometryOccluderList: Geometry[] | null,
    _shadowCache: ShadowCache,
    _ray: Ray,
    _rcvDist: number,
    _srcSize: number,
    _minimumFeatureSize: number,
  ): number {
    return 1.0;
  }

  public static geometryMultiResolutionVisibility(
    _shadowCache: ShadowCache,
    _geometry: Geometry,
    _ray: Ray,
    _rcvDist: number,
    _srcSize: number,
    _minimumFeatureSize: number,
  ): number {
    return 1.0;
  }
}

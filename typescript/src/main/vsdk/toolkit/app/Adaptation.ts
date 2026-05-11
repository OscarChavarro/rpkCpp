import { Cie } from "../common/color/Cie";
import { ColorRgb } from "../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../common/logging/Logger";
import { Numeric } from "../common/linealAlgebra/Numeric";
import { Statistics } from "../common/statistics/Statistics";
import { BsdfComponent } from "../material/BsdfComponent";
import { XxdfComponentFlag } from "../material/XxdfComponentFlag";
import { PatchVisitor } from "../numericalAnalysis/PatchVisitor";
import { Patch } from "../environment/geometry/elements/Patch";
import { ToneMapAdaptationMethod } from "../tonemap/ToneMapAdaptationMethod";
import { ToneMappingContext } from "../tonemap/ToneMappingContext";
import { LuminanceArea } from "./LuminanceArea";

/**
Estimate static adaptation luminance in the current scene
*/
export class Adaptation {
  private static numEntries = 0;
  private static logAreaLum = 0.0;
  private static lumArea: LuminanceArea[] | null = null;
  private static lumAreaIndex = 0;
  private static lumMin = Number.MAX_VALUE; // Note Numeric::HUGE_FLOAT_VALUE; will cause an issue here
  private static lumMax = 0.0;
  private static patchRadianceEstimate: ((patch: Patch) => ColorRgb) | null = null;

  private constructor() {
  }

  /**
  A-priori estimate of a patch's radiance
  */
  private static initRadianceEstimate(patch: Patch): ColorRgb {
    const allXxdfComponents = XxdfComponentFlag.DIFFUSE_COMPONENT
      | XxdfComponentFlag.GLOSSY_COMPONENT
      | XxdfComponentFlag.SPECULAR_COMPONENT;
    const allBsdfComponents = BsdfComponent.BRDF_DIFFUSE_COMPONENT
      | BsdfComponent.BRDF_GLOSSY_COMPONENT
      | BsdfComponent.BRDF_SPECULAR_COMPONENT
      | BsdfComponent.BTDF_DIFFUSE_COMPONENT
      | BsdfComponent.BTDF_GLOSSY_COMPONENT
      | BsdfComponent.BTDF_SPECULAR_COMPONENT;

    const E = PatchVisitor.averageEmittance(patch, allXxdfComponents);
    const R = PatchVisitor.averageNormalAlbedo(patch, allBsdfComponents);
    const radiance = new ColorRgb();

    radiance.scalarProduct(R, Statistics.instance().radiance.estimatedAverageRadiance);
    radiance.addScaled(radiance, (1.0 / globalThis.Math.PI), E);
    return radiance;
  }

  private static patchBrightnessEstimate(patch: Patch): number {
    const radiance = (Adaptation.patchRadianceEstimate as (patch: Patch) => ColorRgb)(patch);
    let brightness = Cie.spectrumLuminance(radiance.r, radiance.g, radiance.b);
    if (brightness < Numeric.EPSILON_FLOAT) {
      brightness = Numeric.EPSILON_FLOAT;
    }
    return brightness;
  }

  private static patchComputeLogAreaLum(patch: Patch): void {
    const brightness = Adaptation.patchBrightnessEstimate(patch);
    // Equation [TUMB1999b](7): log(Lwa) as mean(log(Lw)), here area-weighted over patches
    Adaptation.logAreaLum += patch.area * globalThis.Math.log(brightness);
  }

  private static patchFillLumArea(patch: Patch): void {
    const brightness = Adaptation.patchBrightnessEstimate(patch);

    const entry = (Adaptation.lumArea as LuminanceArea[])[Adaptation.lumAreaIndex];
    entry.luminance = brightness;
    entry.area = patch.area;

    Adaptation.lumMin = globalThis.Math.min(Adaptation.lumMin, entry.luminance);
    Adaptation.lumMax = globalThis.Math.max(Adaptation.lumMax, entry.luminance);

    Adaptation.lumAreaIndex++;
    Adaptation.numEntries++;
  }

  /**
  Computes the static adaptation luminance value choosing the median value
  of area-weighted luminance values. Needs a correct
  Statistics::instance().radiance.totalArea.
  */
  private static meanAreaWeightedLuminance(pairs: LuminanceArea[], numPairs: number): number {
    if (numPairs <= 0) {
      return 0.0;
    }

    const areaMax = Statistics.instance().radiance.totalArea / 2.0;
    let areaCnt = 0.0;
    let pairIndex = 0;

    pairs
      .slice(0, numPairs)
      .sort((la1, la2) => la1.luminance - la2.luminance)
      .forEach((value, index) => {
        pairs[index] = value;
      });

    while (pairIndex < numPairs && areaCnt < areaMax) {
      areaCnt += pairs[pairIndex].area;
      pairIndex++;
    }

    if (pairIndex === 0) {
      return pairs[0].luminance;
    }
    return pairs[pairIndex - 1].luminance;
  }

  /**
  Estimates adaptation luminance in the current scene using the current
  adaption estimation method in toneMapOptions.staticAdaptationMethod
  'patch_radiance' is a pointer to a routine that computes the radiance
  emitted by a patch. The result is filled in toneMapOptions.realWorldAdaptionLuminance
  */
  private static estimateSceneAdaptation(
    patchRadiance: (patch: Patch) => ColorRgb,
    scenePatches: Patch[] | null,
    toneMapOptions: ToneMappingContext
  ): void {
    Adaptation.patchRadianceEstimate = patchRadiance;

    switch (toneMapOptions.staticAdaptationMethod) {
      case ToneMapAdaptationMethod.TMA_NONE:
        break;
      case ToneMapAdaptationMethod.TMA_AVERAGE: {
        // Gibson's static adaptation after [TUMB1999b]
        Adaptation.logAreaLum = 0.0;
        for (let i = 0; scenePatches !== null && i < scenePatches.length; i++) {
          Adaptation.patchComputeLogAreaLum(scenePatches[i]);
        }
        // Equation [TUMB1999b](7): convert mean log-luminance back to luminance domain
        toneMapOptions.realWorldAdaptionLuminance =
          globalThis.Math.exp(Adaptation.logAreaLum / Statistics.instance().radiance.totalArea + 0.84);
        break;
      }
      case ToneMapAdaptationMethod.TMA_MEDIAN: {
        // Static adaptation inspired by [TUMB1999b]
        let pairCount = Statistics.instance().reader.numberOfPatches;
        if (pairCount <= 0) {
          pairCount = scenePatches === null ? 0 : scenePatches.length;
        }
        const la = new Array<LuminanceArea>(pairCount);
        for (let i = 0; i < pairCount; i++) {
          la[i] = new LuminanceArea();
        }

        Adaptation.lumArea = la;
        Adaptation.lumAreaIndex = 0;
        Adaptation.numEntries = 0;
        for (
          let i = 0;
          scenePatches !== null && i < scenePatches.length && Adaptation.lumAreaIndex < pairCount;
          i++
        ) {
          Adaptation.patchFillLumArea(scenePatches[i]);
        }
        toneMapOptions.realWorldAdaptionLuminance = Adaptation.meanAreaWeightedLuminance(la, Adaptation.numEntries);
        break;
      }
      default:
        VsdkLogger.error("sceneBuilderComputeStats", "unknown static adaptation method %s", toneMapOptions.staticAdaptationMethod);
    }
  }

  /**
  Same as Adaptation::estimateSceneAdaptation, but uses some a-priori estimate for the radiance emitted by a patch.
  Used when loading a new scene
  */
  public static initSceneAdaptation(scenePatches: Patch[] | null, toneMapOptions: ToneMappingContext): void {
    Adaptation.estimateSceneAdaptation(Adaptation.initRadianceEstimate, scenePatches, toneMapOptions);
  }
}

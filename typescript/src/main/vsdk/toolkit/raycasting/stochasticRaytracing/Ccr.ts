/**
Constant Control Radiosity
*/

import { ArrayList } from "../../../../java/util/ArrayList";
import { Cie } from "../../common/color/Cie";
import { ColorRgb } from "../../common/color/ColorRgb";
import { Error as VsdkError } from "../../common/Error";
import { Patch } from "../../skin/Patch";
import { McradP } from "./McradP";
import { StochasticRadiosityElement } from "./StochasticRadiosityElement";
import { StochasticRelaxation } from "./StochasticRelaxation";
import { StochasticRaytracingMethod } from "./StochasticRaytracingMethod";

const util = require("node:util");

export class Ccr {
  private constructor() {
  }

  private static readonly NUMBER_OF_INTERVALS = 10;
  private static getRadianceCallback: ((element: StochasticRadiosityElement) => ColorRgb[] | null) | null = null;
  private static getScalingCallback: ((element: StochasticRadiosityElement) => ColorRgb | null) | null = null;

  public static initialControlRadiosityRecursive(
    element: StochasticRadiosityElement,
    minRad: ColorRgb,
    maxRad: ColorRgb,
    fMin: ColorRgb,
    fMax: ColorRgb,
    totalFluxColor: ColorRgb,
    maxRadColor: ColorRgb,
    area: number[]
  ): void {
    void minRad;
    void maxRad;
    void fMin;
    void fMax;

    if (element.regularSubElements === null) {
      const rad = Ccr.getRadianceCallback!(element)![0];
      let weightedArea = element.area;
      if (
        StochasticRelaxation.activeState().importanceDriven !== 0
        && StochasticRelaxation.activeState().method !== StochasticRaytracingMethod.RANDOM_WALK_RADIOSITY_METHOD
      ) {
        weightedArea *= (element.importance - element.sourceImportance);
      }
      totalFluxColor.addScaled(totalFluxColor, weightedArea, rad);
      area[0] += weightedArea;
      maxRadColor.maximum(maxRadColor, rad);
    }
    else {
      for (let i = 0; i < 4; i++) {
        Ccr.initialControlRadiosityRecursive(
          element.regularSubElements[i] as StochasticRadiosityElement,
          minRad,
          maxRad,
          fMin,
          fMax,
          totalFluxColor,
          maxRadColor,
          area
        );
      }
    }
  }

  /**
Initial guess for constant control radiance value
*/
  public static initialControlRadiosity(
    minRad: ColorRgb,
    maxRad: ColorRgb,
    fMin: ColorRgb,
    fMax: ColorRgb,
    scenePatches: ArrayList<Patch>
  ): void {
    const totalFluxColor = new ColorRgb();
    const maxRadColor = new ColorRgb();
    const area = [0.0];
    totalFluxColor.clear();
    maxRadColor.clear();

    for (let i = 0; scenePatches !== null && i < scenePatches.size(); i++) {
      Ccr.initialControlRadiosityRecursive(
        McradP.topLevelStochasticRadiosityElement(scenePatches.get(i)) as StochasticRadiosityElement,
        minRad,
        maxRad,
        fMin,
        fMax,
        totalFluxColor,
        maxRadColor,
        area
      );
    }

    minRad.clear();
    fMin.set(totalFluxColor.r, totalFluxColor.g, totalFluxColor.b);

    maxRad.set(maxRadColor.r, maxRadColor.g, maxRadColor.b);
    fMax.scaledCopy(area[0], maxRadColor);
    fMax.subtract(fMax, totalFluxColor);
  }

  public static refineComponent(
    minRad: number[],
    maxRad: number[],
    fMin: number[],
    fMax: number[],
    f: number[],
    rad: number[]
  ): void {
    let iMin: number;

    fMax[0] = f[0];
    fMin[0] = f[0];
    iMin = 0;
    for (let i = 1; i <= Ccr.NUMBER_OF_INTERVALS; i++) {
      if (f[i] < fMin[0]) {
        fMin[0] = f[i];
        iMin = i;
      }
      if (f[i] > fMax[0]) {
        fMax[0] = f[i];
      }
    }

    if (iMin === 0) {
      minRad[0] = rad[0];
      maxRad[0] = rad[1];
    }
    else if (iMin === Ccr.NUMBER_OF_INTERVALS) {
      minRad[0] = rad[Ccr.NUMBER_OF_INTERVALS - 1];
      maxRad[0] = rad[Ccr.NUMBER_OF_INTERVALS];
    }
    else {
      if (f[iMin - 1] < f[iMin + 1]) {
        minRad[0] = rad[iMin - 1];
        maxRad[0] = rad[iMin];
      }
      else {
        minRad[0] = rad[iMin];
        maxRad[0] = rad[iMin + 1];
      }
    }
  }

  public static refineControlRadiosityRecursive(
    element: StochasticRadiosityElement,
    colorOne: ColorRgb,
    rad: ColorRgb[],
    f: ColorRgb[]
  ): void {
    if (element.regularSubElements === null) {
      const B = Ccr.getRadianceCallback!(element)![0];
      const s = Ccr.getScalingCallback !== null ? Ccr.getScalingCallback(element)! : colorOne;
      let weightedArea = element.area;
      if (
        StochasticRelaxation.activeState().importanceDriven !== 0
        && StochasticRelaxation.activeState().method !== StochasticRaytracingMethod.RANDOM_WALK_RADIOSITY_METHOD
      ) {
        weightedArea *= (element.importance - element.sourceImportance);
      }
      for (let i = 0; i <= Ccr.NUMBER_OF_INTERVALS; i++) {
        const t = new ColorRgb();
        t.scalarProduct(s, rad[i]);
        t.subtract(B, t);
        t.abs();
        f[i].addScaled(f[i], weightedArea, t);
      }
    }
    else {
      for (let i = 0; i < 4; i++) {
        Ccr.refineControlRadiosityRecursive(element.regularSubElements[i] as StochasticRadiosityElement, colorOne, rad, f);
      }
    }
  }

  /**
Finds sub-interval containing optimal constant control radiosity value
Uses regular interval subdivision (generalisation of the bisection
method). Does so component wise
*/
  public static refineControlRadiosity(
    minRad: ColorRgb,
    maxRad: ColorRgb,
    fMin: ColorRgb,
    fMax: ColorRgb,
    scenePatches: ArrayList<Patch>
  ): void {
    const colorOne = new ColorRgb();
    const f = new Array<ColorRgb>(Ccr.NUMBER_OF_INTERVALS + 1);
    const rad = new Array<ColorRgb>(Ccr.NUMBER_OF_INTERVALS + 1);
    const d = new ColorRgb();

    colorOne.setMonochrome(1.0);

    d.subtract(maxRad, minRad);
    for (let i = 0; i <= Ccr.NUMBER_OF_INTERVALS; i++) {
      f[i] = new ColorRgb();
      f[i].clear();
      rad[i] = new ColorRgb();
      rad[i].addScaled(minRad, i / Ccr.NUMBER_OF_INTERVALS, d);
    }

    for (let i = 0; scenePatches !== null && i < scenePatches.size(); i++) {
      Ccr.refineControlRadiosityRecursive(
        McradP.topLevelStochasticRadiosityElement(scenePatches.get(i)) as StochasticRadiosityElement,
        colorOne,
        rad,
        f
      );
    }

    for (let s = 0; s < 3; s++) {
      const fc = new Array<number>(Ccr.NUMBER_OF_INTERVALS + 1);
      const radC = new Array<number>(Ccr.NUMBER_OF_INTERVALS + 1);
      for (let i = 0; i <= Ccr.NUMBER_OF_INTERVALS; i++) {
        switch (s) {
          case 0:
            fc[i] = f[i].r;
            radC[i] = rad[i].r;
            break;
          case 1:
            fc[i] = f[i].g;
            radC[i] = rad[i].g;
            break;
          case 2:
            fc[i] = f[i].b;
            radC[i] = rad[i].b;
            break;
          default:
            break;
        }
      }
      switch (s) {
        case 0: {
          const min = [minRad.r];
          const max = [maxRad.r];
          const fMinC = [fMin.r];
          const fMaxC = [fMax.r];
          Ccr.refineComponent(min, max, fMinC, fMaxC, fc, radC);
          minRad.r = min[0];
          maxRad.r = max[0];
          fMin.r = fMinC[0];
          fMax.r = fMaxC[0];
          break;
        }
        case 1: {
          const min = [minRad.g];
          const max = [maxRad.g];
          const fMinC = [fMin.g];
          const fMaxC = [fMax.g];
          Ccr.refineComponent(min, max, fMinC, fMaxC, fc, radC);
          minRad.g = min[0];
          maxRad.g = max[0];
          fMin.g = fMinC[0];
          fMax.g = fMaxC[0];
          break;
        }
        case 2: {
          const min = [minRad.b];
          const max = [maxRad.b];
          const fMinC = [fMin.b];
          const fMaxC = [fMax.b];
          Ccr.refineComponent(min, max, fMinC, fMaxC, fc, radC);
          minRad.b = min[0];
          maxRad.b = max[0];
          fMin.b = fMinC[0];
          fMax.b = fMaxC[0];
          break;
        }
        default:
          break;
      }
    }
  }

  /**
Determines and returns optimal constant control radiosity value for
the given radiance distribution: this is, the value of beta that
minimises F(beta) = sum over all patches P of P->area times
absolute value of (globalGetRadiance(P) - globalGetScaling(P) * beta).

- getRadiance() returns the radiance to be propagated from a
given ELEMENT.
- getScaling() returns a scale factor (per color component) to be
multiplied with the radiance of the element. If getScaling is a nullptr
pointer, no scaling is applied. Scaling is used in the context of
random walk radiosity
*/
  public static determineControlRadiosity(
    getRadiance: ((element: StochasticRadiosityElement) => ColorRgb[] | null) | null,
    getScaling: ((element: StochasticRadiosityElement) => ColorRgb | null) | null,
    scenePatches: ArrayList<Patch>
  ): ColorRgb {
    const minRad = new ColorRgb();
    const maxRad = new ColorRgb();
    const fMin = new ColorRgb();
    const fMax = new ColorRgb();
    const beta = new ColorRgb();
    const delta = new ColorRgb();
    const eps = 0.001;
    let sweep = 0;

    Ccr.getRadianceCallback = getRadiance;
    Ccr.getScalingCallback = getScaling;
    beta.clear();
    if (Ccr.getRadianceCallback === null) {
      return beta;
    }

    process.stderr.write("Determining optimal control radiosity value ... ");
    Ccr.initialControlRadiosity(minRad, maxRad, fMin, fMax, scenePatches);

    delta.subtract(fMax, fMin);
    delta.addScaled(delta, (-eps), fMin);
    while ((delta.maximumComponent() > 0.0) || sweep < 4) {
      sweep++;
      Ccr.refineControlRadiosity(minRad, maxRad, fMin, fMax, scenePatches);
      delta.subtract(fMax, fMin);
      delta.addScaled(delta, (-eps), fMin);
    }

    beta.add(minRad, maxRad);
    beta.scale(0.5);
    process.stderr.write(`${beta.r} ${beta.g} ${beta.b}`);
    process.stderr.write(util.format(" (%g lux)", globalThis.Math.PI * Cie.spectrumLuminance(beta.r, beta.g, beta.b)));
    process.stderr.write("\n");
    return beta;
  }
}

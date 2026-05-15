import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";
import { RandomWalkEstimatorKind } from "./RandomWalkEstimatorKind";
import { RandomWalkEstimatorType } from "./RandomWalkEstimatorType";
import { Sampler4DSequence } from "./Sampler4DSequence";
import { StochasticRaytracingApproximation } from "./StochasticRaytracingApproximation";
import { StochasticRaytracingMethod } from "./StochasticRaytracingMethod";
import { WhatToShow } from "./WhatToShow";

/**
Used for stochastic relaxation and for random walk radiosity
*/
export class StochasticRelaxation {
  public method: StochasticRaytracingMethod | null;
  public show: WhatToShow | null;
  public inited: number;
  public currentIteration: number;
  public unShotFlux: ColorRgb;
  public totalFlux: ColorRgb;
  public indirectImportanceWeightedUnShotFlux: ColorRgb;
  public unShotYmp: number;
  public totalYmp: number;
  public sourceYmp: number;
  public rayUnitsPerIt: number;
  public bidirectionalTransfers: number;
  public constantControlVariate: number;
  public controlRadiance: ColorRgb;
  public indirectOnly: number;
  public weightedSampling: number;
  public setSource: number;
  public sequence: Sampler4DSequence | null;
  public approximationOrderType: StochasticRaytracingApproximation | null;
  public importanceDriven: number;
  public radianceDriven: number;
  public importanceUpdated: number;
  public importanceUpdatedFromScratch: number;
  public continuousRandomWalk: number;
  public randomWalkEstimatorType: RandomWalkEstimatorType | null;
  public randomWalkEstimatorKind: RandomWalkEstimatorKind | null;
  public randomWalkNumLast: number;
  public discardIncremental: number;
  public incrementalUsesImportance: number;
  public naiveMerging: number;
  public initialNumberOfRays: number;
  public raysPerIteration: number;
  public importanceRaysPerIteration: number;
  public tracedRays: number;
  public prevTracedRays: number;
  public importanceTracedRays: number;
  public prevImportanceTracedRays: number;
  public tracedPaths: number;
  public numberOfMisses: number;
  public doNonDiffuseFirstShot: number;
  public initialLightSourceSamples: number;
  public lastClock: number;
  public cpuSeconds: number;
  public toneMapOptions: ToneMappingContext | null;

  public constructor() {
    this.method = null;
    this.show = null;
    this.inited = 0;
    this.currentIteration = 0;
    this.unShotFlux = new ColorRgb();
    this.totalFlux = new ColorRgb();
    this.indirectImportanceWeightedUnShotFlux = new ColorRgb();
    this.unShotYmp = 0.0;
    this.totalYmp = 0.0;
    this.sourceYmp = 0.0;
    this.rayUnitsPerIt = 0;
    this.bidirectionalTransfers = 0;
    this.constantControlVariate = 0;
    this.controlRadiance = new ColorRgb();
    this.indirectOnly = 0;
    this.weightedSampling = 0;
    this.setSource = 0;
    this.sequence = null;
    this.approximationOrderType = null;
    this.importanceDriven = 0;
    this.radianceDriven = 0;
    this.importanceUpdated = 0;
    this.importanceUpdatedFromScratch = 0;
    this.continuousRandomWalk = 0;
    this.randomWalkEstimatorType = null;
    this.randomWalkEstimatorKind = null;
    this.randomWalkNumLast = 0;
    this.discardIncremental = 0;
    this.incrementalUsesImportance = 0;
    this.naiveMerging = 0;
    this.initialNumberOfRays = 0;
    this.raysPerIteration = 0;
    this.importanceRaysPerIteration = 0;
    this.tracedRays = 0;
    this.prevTracedRays = 0;
    this.importanceTracedRays = 0;
    this.prevImportanceTracedRays = 0;
    this.tracedPaths = 0;
    this.numberOfMisses = 0;
    this.doNonDiffuseFirstShot = 0;
    this.initialLightSourceSamples = 0;
    this.lastClock = 0;
    this.cpuSeconds = 0.0;
    this.toneMapOptions = null;
  }

  public static setActiveState(state: StochasticRelaxation): void {
    StochasticRelaxation.activeStatePtr = state;
  }

  public static activeState(): StochasticRelaxation {
    if (StochasticRelaxation.activeStatePtr === null) {
      VsdkLogger.fatal(-1, "StochasticRelaxation::activeState", "Stochastic relaxation state was not initialized");
    }
    return StochasticRelaxation.activeStatePtr!;
  }

  private static activeStatePtr: StochasticRelaxation | null = null;
}

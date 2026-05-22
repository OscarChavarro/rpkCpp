#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "common/logging/Logger.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

StochasticRelaxation::StochasticRelaxation():
    method(STCHS_RLXTN_RDSTY_MTHD),
    show(SHOW_TOTAL_RADIANCE),
    inited(),
    currentIteration(),
    unShotFlux(),
    totalFlux(),
    indrcImpWghtdUnShotFlux(),
    unShotYmp(),
    totalYmp(),
    sourceYmp(),
    rayUnitsPerIt(),
    bidirectionalTransfers(),
    constantControlVariate(),
    controlRadiance(),
    indirectOnly(),
    weightedSampling(),
    setSource(),
    sequence(RANDOM),
    approximationOrderType(CONSTANT),
    importanceDriven(),
    radianceDriven(),
    importanceUpdated(),
    importanceUpdatedFromScratch(),
    continuousRandomWalk(),
    randomWalkEstimatorType(RW_SHOOTING),
    randomWalkEstimatorKind(RW_COLLISION),
    randomWalkNumLast(),
    discardIncremental(),
    incrementalUsesImportance(),
    naiveMerging(),
    initialNumberOfRays(),
    raysPerIteration(),
    importanceRaysPerIteration(),
    tracedRays(),
    prevTracedRays(),
    importanceTracedRays(),
    prevImportanceTracedRays(),
    tracedPaths(),
    numberOfMisses(),
    doNonDiffuseFirstShot(),
    initialLightSourceSamples(),
    lastClock(),
    cpuSeconds(),
    toneMapOptions()
{
    toneMapOptions = NULL;
}

void
StochasticRelaxation::setActiveState(StochasticRelaxation &state) {
    activeStatePtr() = &state;
}

StochasticRelaxation &
StochasticRelaxation::activeState() {
    StochasticRelaxation *state = activeStatePtr();
    if ( state == NULL ) {
        Logger::fatal(-1, "StochasticRelaxation::activeState", "Stochastic relaxation state was not initialized");
    }
    return *state;
}

StochasticRelaxation *&
StochasticRelaxation::activeStatePtr() {
    static StochasticRelaxation *activeState = NULL;
    return activeState;
}

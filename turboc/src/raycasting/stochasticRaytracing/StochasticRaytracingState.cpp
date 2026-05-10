#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "common/logging/Logger.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

StochasticRelaxation::StochasticRelaxation():
    method(),
    show(),
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
    sequence(),
    approximationOrderType(),
    importanceDriven(),
    radianceDriven(),
    importanceUpdated(),
    importanceUpdatedFromScratch(),
    continuousRandomWalk(),
    randomWalkEstimatorType(),
    randomWalkEstimatorKind(),
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

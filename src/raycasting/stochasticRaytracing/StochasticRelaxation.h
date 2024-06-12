#ifndef __STOCHASTIC_RAYTRACING_STATE__
#define __STOCHASTIC_RAYTRACING_STATE__

#include <ctime>

#include "common/ColorRgb.h"
#include "raycasting/stochasticRaytracing/sample4d.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingApproximation.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingMethod.h"
#include "raycasting/stochasticRaytracing/RandomWalkEstimatorType.h"
#include "raycasting/stochasticRaytracing/RandomWalkEstimatorKind.h"
#include "raycasting/stochasticRaytracing/WhatToShow.h"

/**
Used for stochastic relaxation and for random walk radiosity
*/
class StochasticRelaxation {
  public:
    StochasticRaytracingMethod method; // Stochastic relaxation or random walks
    WhatToShow show; // What to show and how to display the result
    int inited; // Flag indicating whether initialised or not
    int currentIteration;
    ColorRgb unShotFlux;
    ColorRgb totalFlux;
    ColorRgb indirectImportanceWeightedUnShotFlux;
    float unShotYmp;
    float totalYmp; // Sum over all patches of area * importance
    float sourceYmp;
    int rayUnitsPerIt; // To increase or decrease initial nr of rays
    int bidirectionalTransfers; // For bidirectional energy transfers
    int constantControlVariate; // For constant control variate variance reduction
    ColorRgb controlRadiance; // Constant control radiance value
    int indirectOnly; // If to compute indirect illumination only
    int weightedSampling; // If to do weighted sampling ala Powell and Swann / Spanier
    int setSource; // For copying direct illumination to SOURCE_RAD(..) if computing only indirect illumination
    Sampler4DSequence sequence; // Random number sequence
    StochasticRaytracingApproximation approximationOrderType; // Radiosity approximation order
    int importanceDriven; // If to use view-importance
    int radianceDriven; // Radiance-driven importance propagation
    int importanceUpdated; // Direct importance got updated or not?
    int importanceUpdatedFromScratch; // Can either incremental or from scratch
    int continuousRandomWalk; // Continuous or discrete random walk
    RandomWalkEstimatorType randomWalkEstimatorType; // Shooting, gathering, gathering for free
    RandomWalkEstimatorKind randomWalkEstimatorKind; // Collision, absorption, ...
    int randomWalkNumLast; // For last-but-n and n-last RW estimators
    int discardIncremental; // First iteration (incremental steps) results are discarded in later computations
    int incrementalUsesImportance; // View-importance is used already for the first iteration (incremental steps). This may confuse the merging heuristic or other things
    int naiveMerging; // Results of different iterations are merged solely based on the number of rays shot
    long initialNumberOfRays; // For first iteration step
    long raysPerIteration; // For later iterations
    long importanceRaysPerIteration; // For later iterations for importance
    long tracedRays; // Total number of traced rays
    long prevTracedRays; // Previous total of traced rays
    long importanceTracedRays;
    long prevImportanceTracedRays;
    long tracedPaths; // Number of traced random walks in random walk rad.
    long numberOfMisses; // Rays disappearing to background
    int doNonDiffuseFirstShot; // Initial shooting pass handles non-diffuse lights
    int initialLightSourceSamples; // Initial shot samples per light source
    clock_t lastClock; // For computation timings
    float cpuSeconds; // CPU time spent in calculations

    StochasticRelaxation();
};

extern StochasticRelaxation GLOBAL_stochasticRaytracing_monteCarloRadiosityState;

#endif

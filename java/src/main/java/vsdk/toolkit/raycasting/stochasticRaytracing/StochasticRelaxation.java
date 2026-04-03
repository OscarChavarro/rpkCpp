package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.tonemap.ToneMappingContext;

/**
Used for stochastic relaxation and for random walk radiosity
*/
public class StochasticRelaxation {
    public StochasticRaytracingMethod method; // Stochastic relaxation or random walks
    public WhatToShow show; // What to show and how to display the result
    public int inited; // Flag indicating whether initialised or not
    public int currentIteration;
    public ColorRgb unShotFlux;
    public ColorRgb totalFlux;
    public ColorRgb indirectImportanceWeightedUnShotFlux;
    public float unShotYmp;
    public float totalYmp; // Sum over all patches of area * importance
    public float sourceYmp;
    public int rayUnitsPerIt; // To increase or decrease initial nr of rays
    public int bidirectionalTransfers; // For bidirectional energy transfers
    public int constantControlVariate; // For constant control variate variance reduction
    public ColorRgb controlRadiance; // Constant control radiance value
    public int indirectOnly; // If to compute indirect illumination only
    public int weightedSampling; // If to do weighted sampling ala Powell and Swann / Spanier
    public int setSource; // For copying direct illumination to SOURCE_RAD(..) if computing only indirect illumination
    public Sampler4DSequence sequence; // Random number sequence
    public StochasticRaytracingApproximation approximationOrderType; // Radiosity approximation order
    public int importanceDriven; // If to use view-importance
    public int radianceDriven; // Radiance-driven importance propagation
    public int importanceUpdated; // Direct importance got updated or not?
    public int importanceUpdatedFromScratch; // Can either incremental or from scratch
    public int continuousRandomWalk; // Continuous or discrete random walk
    public RandomWalkEstimatorType randomWalkEstimatorType; // Shooting, gathering, gathering for free
    public RandomWalkEstimatorKind randomWalkEstimatorKind; // Collision, absorption, ...
    public int randomWalkNumLast; // For last-but-n and n-last RW estimators
    public int discardIncremental; // First iteration (incremental steps) results are discarded in later computations
    public int incrementalUsesImportance; // View-importance is used already for the first iteration (incremental steps). This may confuse the merging heuristic or other things
    public int naiveMerging; // Results of different iterations are merged solely based on the number of rays shot
    public long initialNumberOfRays; // For first iteration step
    public long raysPerIteration; // For later iterations
    public long importanceRaysPerIteration; // For later iterations for importance
    public long tracedRays; // Total number of traced rays
    public long prevTracedRays; // Previous total of traced rays
    public long importanceTracedRays;
    public long prevImportanceTracedRays;
    public long tracedPaths; // Number of traced random walks in random walk rad.
    public long numberOfMisses; // Rays disappearing to background
    public int doNonDiffuseFirstShot; // Initial shooting pass handles non-diffuse lights
    public int initialLightSourceSamples; // Initial shot samples per light source
    public long lastClock; // For computation timings (nanoseconds)
    public float cpuSeconds; // CPU time spent in calculations
    public ToneMappingContext toneMapOptions;

    public StochasticRelaxation() {
        method = null;
        show = null;
        inited = 0;
        currentIteration = 0;
        unShotFlux = new ColorRgb();
        totalFlux = new ColorRgb();
        indirectImportanceWeightedUnShotFlux = new ColorRgb();
        unShotYmp = 0.0f;
        totalYmp = 0.0f;
        sourceYmp = 0.0f;
        rayUnitsPerIt = 0;
        bidirectionalTransfers = 0;
        constantControlVariate = 0;
        controlRadiance = new ColorRgb();
        indirectOnly = 0;
        weightedSampling = 0;
        setSource = 0;
        sequence = null;
        approximationOrderType = null;
        importanceDriven = 0;
        radianceDriven = 0;
        importanceUpdated = 0;
        importanceUpdatedFromScratch = 0;
        continuousRandomWalk = 0;
        randomWalkEstimatorType = null;
        randomWalkEstimatorKind = null;
        randomWalkNumLast = 0;
        discardIncremental = 0;
        incrementalUsesImportance = 0;
        naiveMerging = 0;
        initialNumberOfRays = 0L;
        raysPerIteration = 0L;
        importanceRaysPerIteration = 0L;
        tracedRays = 0L;
        prevTracedRays = 0L;
        importanceTracedRays = 0L;
        prevImportanceTracedRays = 0L;
        tracedPaths = 0L;
        numberOfMisses = 0L;
        doNonDiffuseFirstShot = 0;
        initialLightSourceSamples = 0;
        lastClock = 0L;
        cpuSeconds = 0.0f;
        toneMapOptions = null;
    }

    public static void setActiveState(StochasticRelaxation state) {
        activeStatePtr = state;
    }

    public static StochasticRelaxation activeState() {
        if ( activeStatePtr == null ) {
            Error.fatal(-1, "StochasticRelaxation::activeState", "Stochastic relaxation state was not initialized");
        }
        return activeStatePtr;
    }

    private static StochasticRelaxation activeStatePtr = null;
}

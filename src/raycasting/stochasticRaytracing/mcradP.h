#ifndef __MONTE_CARLO_RADIOSITY__
#define __MONTE_CARLO_RADIOSITY__

#include <ctime>

#include "java/util/ArrayList.h"
#include "scene/Scene.h"
#include "raycasting/stochasticRaytracing/sample4d.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "raycasting/stochasticRaytracing/coefficientsmcrad.h"

/**
Global variables: same set for stochastic relaxation and for random walk radiosity
*/

enum METHOD {
    STOCHASTIC_RELAXATION_RADIOSITY_METHOD,
    RANDOM_WALK_RADIOSITY_METHOD
};

enum RANDOM_WALK_ESTIMATOR_TYPE {
    RW_SHOOTING,
    RW_GATHERING
};

enum RANDOM_WALK_ESTIMATOR_KIND {
    RW_COLLISION,
    RW_ABSORPTION,
    RW_SURVIVAL,
    RW_LAST_BUT_NTH,
    RW_N_LAST
};

enum SHOW_WHAT {
    SHOW_TOTAL_RADIANCE,
    SHOW_INDIRECT_RADIANCE,
    SHOW_IMPORTANCE
};

class STATE {
  public:
    METHOD method; // Stochastic relaxation or random walks
    SHOW_WHAT show; // What to show and how to display the result
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
    int weightedSampling; // If to do weighted sampling ala Powell and Swann/Spanier
    int setSource; // For copying direct illumination to SOURCE_RAD(..) if computing only indirect illumination
    SEQ4D sequence; // Random number sequence, see sample4d.h
    APPROX_TYPE approximationOrderType; // Radiosity approximation order
    int importanceDriven; // If to use view-importance
    int radianceDriven; // Radiance-driven importance propagation
    int importanceUpdated; // Direct importance got updated or not?
    int importanceUpdatedFromScratch; // Can either incremental or from scratch
    int continuousRandomWalk; // Continuous or discrete random walk
    RANDOM_WALK_ESTIMATOR_TYPE randomWalkEstimatorType; // Shooting, gathering, gathering for free
    RANDOM_WALK_ESTIMATOR_KIND randomWalkEstimatorKind; // Collision, absorption, ...
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

    STATE():
        method(),
        show(),
        inited(),
        currentIteration(),
        unShotFlux(),
        totalFlux(),
        indirectImportanceWeightedUnShotFlux(),
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
        cpuSeconds()
    {}
};

extern STATE GLOBAL_stochasticRaytracing_monteCarloRadiosityState;

inline int
NR_VERTICES(StochasticRadiosityElement *elem) {
    return elem->patch->numberOfVertices;
}

inline StochasticRadiosityElement*
topLevelGalerkinElement(Patch *patch) {
    return (StochasticRadiosityElement *)patch->radianceData;
}

inline ColorRgb *
getTopLevelPatchRad(Patch *patch) {
    return topLevelGalerkinElement(patch)->radiance;
}

inline ColorRgb *
getTopLevelPatchUnShotRad(Patch *patch) {
    return topLevelGalerkinElement(patch)->unShotRadiance;
}

inline ColorRgb*
getTopLevelPatchReceivedRad(Patch *patch) {
    return topLevelGalerkinElement(patch)->receivedRadiance;
}

inline GalerkinBasis *
getTopLevelPatchBasis(Patch *patch) {
    return topLevelGalerkinElement(patch)->basis;
}

extern float monteCarloRadiosityScalarReflectance(Patch *P);
extern void monteCarloRadiosityDefaults();
extern void monteCarloRadiosityUpdateCpuSecs();
extern Element *monteCarloRadiosityCreatePatchData(Patch *patch);
extern void monteCarloRadiosityDestroyPatchData(Patch *patch);
extern void monteCarloRadiosityPatchComputeNewColor(Patch *patch);
extern void monteCarloRadiosityInit();
extern void monteCarloRadiosityUpdateViewImportance(Scene *scene);
extern void monteCarloRadiosityReInit(Scene *scene);
extern void monteCarloRadiosityPreStep(Scene *scene);
extern void monteCarloRadiosityTerminate(java::ArrayList<Patch *> *scenePatches);
extern ColorRgb monteCarloRadiosityGetRadiance(Patch *patch, double u, double v, Vector3D dir);
extern void stochasticRelaxationRadiosityParseOptions(int *argc, char **argv);
extern void randomWalkRadiosityParseOptions(int *argc, char **argv);
extern void doNonDiffuseFirstShot(Scene *scene, RadianceMethod *context);

#endif

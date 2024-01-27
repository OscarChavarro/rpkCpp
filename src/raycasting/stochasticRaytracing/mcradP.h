#ifndef __MONTE_CARLO_RADIOSITY__
#define __MONTE_CARLO_RADIOSITY__

#include <ctime>

#include "java/util/ArrayList.h"
#include "raycasting/stochasticRaytracing/sample4d.h"
#include "raycasting/stochasticRaytracing/elementmcrad.h"
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
    METHOD method; // stochastic relaxation or random walks
    SHOW_WHAT show; // what to show and how to display the result

    int inited; // flag indicating whether initialised or not

    int currentIteration;
    COLOR unShotFlux;
    COLOR totalFlux;
    COLOR indirectImportanceWeightedUnShotFlux;
    float unShotYmp;
    float totalYmp; // Sum over all patches of area * importance
    float sourceYmp;

    int rayUnitsPerIt; // to increase or decrease initial nr of rays
    int bidirectionalTransfers; // for bidirectional energy transfers
    int constantControlVariate; // for constant control variate variance reduction
    COLOR controlRadiance; // constant control radiance value
    int indirectOnly; // If to compute indirect illum. only

    int weightedSampling; // If to do weighted sampling ala Powell and Swann/Spanier

    int setSource; // for copying direct illumination to SOURCE_RAD(..) if computing only indirect illumination
    SEQ4D sequence; // random number sequence, see sample4d.h
    APPROX_TYPE approximationOrderType; // radiosity approximation order
    int importanceDriven; // If to use view-importance
    int radianceDriven; // radiance-driven importance propagation
    int importanceUpdated; // direct importance got updated or not?
    int importanceUpdatedFromScratch; // can either incremental or from scratch
    int continuousRandomWalk; // continuous or discrete random walk
    RANDOM_WALK_ESTIMATOR_TYPE randomWalkEstimatorType; // shooting, gathering, gathering for free
    RANDOM_WALK_ESTIMATOR_KIND randomWalkEstimatorKind; // collision, absorption, ...
    int randomWalkNumLast; // for last-but-n and n-last RW estimators
    int discardIncremental; // first iteration (incremental steps) results are discarded in later computations
    int incrementalUsesImportance; // view-importance is used already for the first iteration (incremental steps). This may confuse the merging heuristic or other things
    int naiveMerging; // results of different iterations are merged solely based on the number of rays shot

    long initialNumberOfRays; // for first iteration step
    long raysPerIteration; // for later iterations
    long importanceRaysPerIteration; // for later iterations for importance
    long tracedRays; // total nr of traced rays
    long prevTracedRays; // previous total of traced rays
    long importanceTracedRays;
    long prevImportanceTracedRays;
    long tracedPaths; // nr of traced random walks in random walk rad.
    long numberOfMisses; // rays disappearing to background
    int doNonDiffuseFirstShot; // initial shooting pass handles non-diffuse lights
    int initialLightSourceSamples; // initial shot samples per light source

    clock_t lastClock; // for computation timings
    float cpuSeconds; // CPU time spent in calculations
};

extern STATE GLOBAL_stochasticRaytracing_monteCarloRadiosityState;

inline int
NR_VERTICES(StochasticRadiosityElement *elem) {
    return elem->patch->numberOfVertices;
}

inline StochasticRadiosityElement*
TOPLEVEL_ELEMENT(Patch *patch) {
    return (StochasticRadiosityElement *)patch->radianceData;
}

inline COLOR *
getTopLevelPatchRad(Patch *patch) {
    return TOPLEVEL_ELEMENT(patch)->rad;
}

inline COLOR *
getTopLevelPatchUnShotRad(Patch *patch) {
    return TOPLEVEL_ELEMENT(patch)->unshot_rad;
}

inline COLOR*
getTopLevelPatchReceivedRad(Patch *patch) {
    return TOPLEVEL_ELEMENT(patch)->received_rad;
}

inline GalerkinBasis *
getTopLevelPatchBasis(Patch *patch) {
    return TOPLEVEL_ELEMENT(patch)->basis;
}

extern float monteCarloRadiosityScalarReflectance(Patch *P);
extern void monteCarloRadiosityDefaults();
extern void monteCarloRadiosityUpdateCpuSecs();
extern Element *monteCarloRadiosityCreatePatchData(Patch *patch);
extern void monteCarloRadiosityPrintPatchData(FILE *out, Patch *patch);
extern void monteCarloRadiosityDestroyPatchData(Patch *patch);
extern void monteCarloRadiosityPatchComputeNewColor(Patch *patch);
extern void monteCarloRadiosityInit();
extern void monteCarloRadiosityUpdateViewImportance(java::ArrayList<Patch *> *scenePatches);
extern void monteCarloRadiosityReInit(java::ArrayList<Patch *> *scenePatches);
extern void monteCarloRadiosityPreStep(java::ArrayList<Patch *> *scenePatches);
extern void monteCarloRadiosityTerminate();
extern COLOR monteCarloRadiosityGetRadiance(Patch *patch, double u, double v, Vector3D dir);
extern void monteCarloRadiosityRecomputeDisplayColors();
extern void monteCarloRadiosityUpdateMaterial(Material *oldMaterial, Material *newMaterial);

extern void stochasticRelaxationRadiosityParseOptions(int *argc, char **argv);
extern void stochasticRelaxationRadiosityPrintOptions(FILE *fp);

extern void randomWalkRadiosityParseOptions(int *argc, char **argv);
extern void randomWalkRadiosityPrintOptions(FILE *fp);

extern void doNonDiffuseFirstShot();

#endif

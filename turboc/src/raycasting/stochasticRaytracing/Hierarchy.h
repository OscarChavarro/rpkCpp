#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

/**
Hierarchical refinement stuff (includes Jan's elementP.h)
*/

#ifndef __ELEMENT_HIERARCHY__
#define __ELEMENT_HIERARCHY__

#include "java/util/ArrayList.h"
#include "common/statistics/Statistics.h"
#include "raycasting/stochasticRaytracing/Link.h"
#include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "raycasting/stochasticRaytracing/HierarchyClusteringMode.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

class Hierarchy{ public:
    static REFINE_ACTION powerOracle(const Link *link);
    static Link topLink(StochasticRadiosityElement *rcvTop, StochasticRadiosityElement *srcTop);
    static Link *hierarchyRefine( Link *link, StochasticRadiosityElement *rcvTop, double *ur, double *vr, StochasticRadiosityElement *srcTop, double *us, double *vs, ORACLE evaluateLink, const RenderOptions *renderOptions);

    static void elementHierarchyDefaults();
    static void elementHierarchyInit(Geometry *clusteredWorldGeometry);
    static void elementHierarchyTerminate(const ArrayList<Patch *> *scenePatches);

  private:
    #define DEFAULT_EH_EPSILON ((float)5e-4)
    #define DEFAULT_EH_MINIMUM_AREA ((float)1e-6)
    #define DEFAULT_EH_HIERARCHICAL_MESHING true
    #define DEFAULT_EH_T_VERTEX_ELIMINATION true
    #define DEFAULT_EH_CLUSTERING ORIENTED_CLUSTERING

    static Link *dontRefineCallBack( Link *link, StochasticRadiosityElement *rcvtop, double *ur, double *vr, StochasticRadiosityElement *srctop, double *us, double *vs, const RenderOptions *renderOptions);
    static Link *subdivideReceiverCallBack( Link *link, StochasticRadiosityElement *rcvtop, double *ur, double *vr, StochasticRadiosityElement *srctop, double *us, double *vs, const RenderOptions *renderOptions);
    static Link *subdivideSourceCallBack( Link *link, StochasticRadiosityElement *rcvtop, double *ur, double *vr, StochasticRadiosityElement *srctop, double *us, double *vs, const RenderOptions *renderOptions);
    static bool selfLink(const Link *link);
    static float formFactorEstimate(const StochasticRadiosityElement *rcv, const StochasticRadiosityElement *src);
    static bool lowPowerLink(const Link *link, const Statistics *statistics);
    static REFINE_ACTION subDivideLargest(const Link *link);
};

#endif

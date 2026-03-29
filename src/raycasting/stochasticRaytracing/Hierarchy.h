/**
Hierarchical refinement stuff (includes Jan's elementP.h)
*/

#ifndef __ELEMENT_HIERARCHY__
#define __ELEMENT_HIERARCHY__

#include "java/util/ArrayList.h"
#include "raycasting/stochasticRaytracing/Link.h"
#include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "raycasting/stochasticRaytracing/HierarchyClusteringMode.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

class Statistics;

class Hierarchy final {
  public:
    static REFINE_ACTION powerOracle(const Link *link);
    static Link topLink(StochasticRadiosityElement *rcvTop, StochasticRadiosityElement *srcTop);
    static Link *hierarchyRefine(
        Link *link,
        StochasticRadiosityElement *rcvTop,
        double *ur,
        double *vr,
        StochasticRadiosityElement *srcTop,
        double *us,
        double *vs,
        ORACLE evaluateLink,
        const RenderOptions *renderOptions);

    static void elementHierarchyDefaults();
    static void elementHierarchyInit(Geometry *clusteredWorldGeometry);
    static void elementHierarchyTerminate(const java::ArrayList<Patch *> *scenePatches);

  private:
    static Link *dontRefineCallBack(
        Link *link,
        StochasticRadiosityElement *rcvtop,
        double *ur,
        double *vr,
        StochasticRadiosityElement *srctop,
        double *us,
        double *vs,
        const RenderOptions *renderOptions);
    static Link *subdivideReceiverCallBack(
        Link *link,
        StochasticRadiosityElement *rcvtop,
        double *ur,
        double *vr,
        StochasticRadiosityElement *srctop,
        double *us,
        double *vs,
        const RenderOptions *renderOptions);
    static Link *subdivideSourceCallBack(
        Link *link,
        StochasticRadiosityElement *rcvtop,
        double *ur,
        double *vr,
        StochasticRadiosityElement *srctop,
        double *us,
        double *vs,
        const RenderOptions *renderOptions);
    static int selfLink(const Link *link);
    static float formFactorEstimate(const StochasticRadiosityElement *rcv, const StochasticRadiosityElement *src);
    static int lowPowerLink(const Link *link, const Statistics *statistics);
    static REFINE_ACTION subDivideLargest(const Link *link);
};

extern ElementHierarchyState GLOBAL_stochasticRaytracing_hierarchy;

#endif

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
    static constexpr float DEFAULT_EH_EPSILON = 5e-4f;
    static constexpr float DEFAULT_EH_MINIMUM_AREA = 1e-6f;
    static constexpr bool DEFAULT_EH_HIERARCHICAL_MESHING = true;
    static constexpr bool DEFAULT_EH_T_VERTEX_ELIMINATION = true;
    static constexpr HierarchyClusteringMode DEFAULT_EH_CLUSTERING = HierarchyClusteringMode::ORIENTED_CLUSTERING;

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
    static bool selfLink(const Link *link);
    static float formFactorEstimate(const StochasticRadiosityElement *rcv, const StochasticRadiosityElement *src);
    static bool lowPowerLink(const Link *link, const Statistics *statistics);
    static REFINE_ACTION subDivideLargest(const Link *link);
};

#endif

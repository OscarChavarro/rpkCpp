#ifndef __STOCHASTIC_JACOBI__
#define __STOCHASTIC_JACOBI__

#include "java/util/ArrayList.h"
#include "common/ColorRgb.h"
#include "common/RenderOptions.h"
#include "skin/Patch.h"
#include "scene/VoxelGrid.h"
#include "raycasting/stochasticRaytracing/Link.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

/**
Generic routine for Stochastic Jacobi iterations:
- nr_rays: nr of rays to use
- getRadianceCallBack: routine returning radiance (total or un-shot) to be
propagated for a given element, or nullptr if not radiance propagation is
required.
- getImportanceCallBack: same, but for importance.
- updateCallBack: routine updating total, un-shot and source radiance and/or
importance based on result received during the iteration.

The operation of this routine is further controlled by global parameters
- GLOBAL_stochasticRaytracing_monteCarloRadiosityState.do_control_radiosity: perform constant control variate variance reduction
- GLOBAL_stochasticRaytracing_monteCarloRadiosityState.bidirectionalTransfers: for using lines bidirectionally
- GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven: importance-driven radiance propagation
- GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven: radiance-driven importance propagation
- hierarchy.do_h_meshing, hierarchy.clustering: hierarchical refinement/clustering

This routine updates global ray counts and total/un-shot power/importance statistics.

CAVEAT: propagate either radiance or importance alone. Simultaneous
propagation of importance and radiance does not work yet.
*/
class Stochjacobi final {
  public:
    static void doStochasticJacobiIteration(
        VoxelGrid *sceneWorldVoxelGrid,
        long numberOfRays,
        ColorRgb *(*getRadianceCallBack)(const StochasticRadiosityElement *),
        float (*getImportanceCallBack)(const StochasticRadiosityElement *),
        void updateCallBack(StochasticRadiosityElement *elem, double w),
        const java::ArrayList<Patch *> *scenePatches,
        RenderOptions *renderOptions);

  private:
    static void stochasticJacobiInitGlobals(
        int numberOfRays,
        ColorRgb *(*getRadianceCallBack)(const StochasticRadiosityElement *),
        float (*getImportanceCallBack)(const StochasticRadiosityElement *),
        void (*updateCallBack)(StochasticRadiosityElement *elem, double w));
    static void stochasticJacobiPrintMessage(long numberOfRays);
    static double stochasticJacobiProbability(const StochasticRadiosityElement *elem);
    static void stochasticJacobiElementClearAccumulators(StochasticRadiosityElement *elem);
    static void stochasticJacobiElementSetup(Element *element);
    static bool stochasticJacobiSetup(const java::ArrayList<Patch *> *scenePatches);
    static ColorRgb stochasticJacobiGetSourceRadiance(const StochasticRadiosityElement *src, double us, double vs);
    static void stochasticJacobiPropagateRadianceToSurface(
        StochasticRadiosityElement *rcv,
        double ur,
        double vr,
        ColorRgb rayPower,
        const StochasticRadiosityElement *src,
        double fraction,
        double weight);
    static void stochasticJacobiPropagateRadianceToClusterIsotropic(
        StochasticRadiosityElement *cluster,
        ColorRgb rayPower,
        const StochasticRadiosityElement *src,
        double fraction,
        double weight);
    static void stochasticJacobiPropagateRadianceClusterRecursive(
        StochasticRadiosityElement *currentElement,
        ColorRgb rayPower,
        Ray *ray,
        float dir,
        double projectedArea,
        double fraction);
    static void stochasticJacobiPropagateRadianceToClusterOriented(
        StochasticRadiosityElement *cluster,
        ColorRgb rayPower,
        Ray *ray,
        float dir,
        const StochasticRadiosityElement *src,
        double projectedArea,
        double fraction,
        double weight);
    static void stochasticJacobiPropagateRadiance(
        const StochasticRadiosityElement *src,
        double us,
        double vs,
        StochasticRadiosityElement *rcv,
        double ur,
        double vr,
        double src_prob,
        double rcv_prob,
        Ray *ray,
        float dir);
    static void stochasticJacobiPropagateImportance(
        const StochasticRadiosityElement *src,
        double us,
        double vs,
        StochasticRadiosityElement *rcv,
        double ur,
        double vr,
        double src_prob,
        double rcv_prob,
        const Ray *ray,
        float dir);
    static double stochasticJacobiReceiverProjectedArea(
        const StochasticRadiosityElement *cluster,
        Ray *ray,
        float dir);
    static void stochasticJacobiReceiverProjectedAreaRecursive(
        const StochasticRadiosityElement *currentElement,
        Ray *ray,
        float dir,
        double *area);
    static void stochasticJacobiRefineAndPropagateRadiance(
        const StochasticRadiosityElement *src,
        double us,
        double vs,
        StochasticRadiosityElement *P,
        double up,
        double vp,
        StochasticRadiosityElement *Q,
        double uq,
        double vq,
        double src_prob,
        double rcv_prob,
        Ray *ray,
        float dir,
        const RenderOptions *renderOptions);
    static void stochasticJacobiRefineAndPropagateImportance(
        const StochasticRadiosityElement *P,
        double up,
        double vp,
        StochasticRadiosityElement *Q,
        double uq,
        double vq,
        double src_prob,
        double rcv_prob,
        const Ray *ray,
        float dir);
    static void stochasticJacobiRefineAndPropagate(
        StochasticRadiosityElement *P,
        double up,
        double vp,
        StochasticRadiosityElement *Q,
        double uq,
        double vq,
        Ray *ray,
        const RenderOptions *renderOptions);
    static double *stochasticJacobiNextSample(
        StochasticRadiosityElement *elem,
        int nMostSignificantBit,
        NiederreiterIndex mostSignificantBit1,
        NiederreiterIndex rMostSignificantBit2,
        double *zeta);
    static void stochasticJacobiUniformHitCoordinates(const RayHit *hit, double *uHit, double *vHit);
    static void stochasticJacobiElementShootRay(
        const VoxelGrid *sceneWorldVoxelGrid,
        StochasticRadiosityElement *src,
        int nMostSignificantBit,
        NiederreiterIndex mostSignificantBit1,
        NiederreiterIndex rMostSignificantBit2,
        const RenderOptions *renderOptions);
    static void stochasticJacobiElementShootRays(
        const VoxelGrid *sceneWorldVoxelGrid,
        StochasticRadiosityElement *element,
        int raysThisElem,
        const RenderOptions *renderOptions);
    static void stochasticJacobiShootRaysRecursive(
        VoxelGrid *sceneWorldVoxelGrid,
        StochasticRadiosityElement *element,
        double rnd,
        long *rayCount,
        double *cumulative,
        RenderOptions *renderOptions);
    static void stochasticJacobiShootRays(
        VoxelGrid *sceneWorldVoxelGrid,
        const java::ArrayList<Patch *> *scenePatches,
        RenderOptions *renderOptions);
    static void stochasticJacobiUpdateElement(StochasticRadiosityElement *elem);
    static void stochasticJacobiPush(const StochasticRadiosityElement *parent, StochasticRadiosityElement *child);
    static void stochasticJacobiPull(StochasticRadiosityElement *parent, const StochasticRadiosityElement *child);
    static void stochasticJacobiPushUpdatePullChild(Element *element);
    static void stochasticJacobiPushUpdatePull(Element *element);
    static void stochasticJacobiClearElement(StochasticRadiosityElement *parent);
    static void stochasticJacobiPullRdEdFromChild(Element *element);
    static void stochasticJacobiPullRdEd(StochasticRadiosityElement *element);
    static void stochasticJacobiPushUpdatePullSweep();
    static void stochasticJacobiInitPushRayIndex(Element *element);
};

#endif

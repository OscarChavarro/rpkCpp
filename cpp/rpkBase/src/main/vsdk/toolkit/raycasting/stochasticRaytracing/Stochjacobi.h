#ifndef STOCHASTIC_JACOBI__
#define STOCHASTIC_JACOBI__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Link.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

/**
Generic routine for Stochastic Jacobi iterations:
- nr_rays: nr of rays to use
- getRadianceCallBack: routine returning radiance (total or un-shot) to be
propagated for a given element, or nullptr if not radiance propagation is
required.
- getImportanceCallBack: same, but for importance.
- updateCallBack: routine updating total, un-shot and source radiance and/or
importance based on result received during the iteration.

The operation of this routine is further controlled by stochastic relaxation
state parameters:
- constantControlVariate: perform constant control variate variance reduction
- bidirectionalTransfers: use lines bidirectionally
- importanceDriven: importance-driven radiance propagation
- radianceDriven: radiance-driven importance propagation
- hierarchy.do_h_meshing, hierarchy.clustering: hierarchical refinement/clustering

This routine updates global ray counts and total/un-shot power/importance statistics.

CAVEAT: propagate either radiance or importance alone. Simultaneous
propagation of importance and radiance does not work yet.
*/
class StochasticJacobi final {
  public:
    using GetRadianceCallback = ColorRgbMutable *(*)(const StochasticRadiosityElement *);
    using GetImportanceCallback = float (*)(const StochasticRadiosityElement *);
    using UpdateCallback = void (*)(StochasticRadiosityElement *elem, double w);

    static void doStochasticJacobiIteration(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        long numberOfRays,
        GetRadianceCallback getRadianceCallBack,
        GetImportanceCallback getImportanceCallBack,
        UpdateCallback updateCallBack,
        const java::ArrayList<Patch *> *scenePatches,
        RendererConfiguration *renderOptions);

  private:
    static GetRadianceCallback getRadianceCallback;
    static GetImportanceCallback getImportanceCallback;
    static UpdateCallback reflectCallback;
    static int useControlVariate;
    static int numberOfRaysToShoot;
    static double sumOfProbabilities;

    static void stochasticJacobiInitGlobals(
        int numberOfRays,
        GetRadianceCallback getRadianceCallBack,
        GetImportanceCallback getImportanceCallBack,
        UpdateCallback updateCallBack);
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
        ColorRgbMutable rayPower,
        const StochasticRadiosityElement *src,
        double fraction,
        double weight);
    static void stochasticJacobiPropagateRadianceToClusterIsotropic(
        StochasticRadiosityElement *cluster,
        ColorRgbMutable rayPower,
        const StochasticRadiosityElement *src,
        double fraction,
        double weight);
    static void stochasticJacobiPropagateRadianceClusterRecursive(
        StochasticRadiosityElement *currentElement,
        ColorRgbMutable rayPower,
        RpkRay *ray,
        float dir,
        double projectedArea,
        double fraction);
    static void stochasticJacobiPropagateRadianceToClusterOriented(
        StochasticRadiosityElement *cluster,
        ColorRgbMutable rayPower,
        RpkRay *ray,
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
        RpkRay *ray,
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
        const RpkRay *ray,
        float dir);
    static double stochasticJacobiReceiverProjectedArea(
        const StochasticRadiosityElement *cluster,
        RpkRay *ray,
        float dir);
    static void stochasticJacobiReceiverProjectedAreaRecursive(
        const StochasticRadiosityElement *currentElement,
        RpkRay *ray,
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
        RpkRay *ray,
        float dir,
        const RendererConfiguration *renderOptions);
    static void stochasticJacobiRefineAndPropagateImportance(
        const StochasticRadiosityElement *P,
        double up,
        double vp,
        StochasticRadiosityElement *Q,
        double uq,
        double vq,
        double src_prob,
        double rcv_prob,
        const RpkRay *ray,
        float dir);
    static void stochasticJacobiRefineAndPropagate(
        StochasticRadiosityElement *P,
        double up,
        double vp,
        StochasticRadiosityElement *Q,
        double uq,
        double vq,
        RpkRay *ray,
        const RendererConfiguration *renderOptions);
    static double *stochasticJacobiNextSample(
        StochasticRadiosityElement *elem,
        int nMostSignificantBit,
        NiederreiterIndex mostSignificantBit1,
        NiederreiterIndex rMostSignificantBit2,
        double *zeta);
    static void stochasticJacobiUniformHitCoordinates(const RpkRayHit *hit, double *uHit, double *vHit);
    static void stochasticJacobiElementShootRay(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        StochasticRadiosityElement *src,
        int nMostSignificantBit,
        NiederreiterIndex mostSignificantBit1,
        NiederreiterIndex rMostSignificantBit2,
        const RendererConfiguration *renderOptions);
    static void stochasticJacobiElementShootRays(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        StochasticRadiosityElement *element,
        int raysThisElem,
        const RendererConfiguration *renderOptions);
    static void stochasticJacobiShootRaysRecursive(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        StochasticRadiosityElement *element,
        double rnd,
        long *rayCount,
        double *cumulative,
        RendererConfiguration *renderOptions);
    static void stochasticJacobiShootRays(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        const java::ArrayList<Patch *> *scenePatches,
        RendererConfiguration *renderOptions);
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

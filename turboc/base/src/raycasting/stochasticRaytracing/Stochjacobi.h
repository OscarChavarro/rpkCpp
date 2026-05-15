#ifndef __STOCHASTIC_JACOBI__
#define __STOCHASTIC_JACOBI__

#include "java/util/ArrayList.h"
#include "common/color/ColorRgb.h"
#include "material/RendererConfiguration.h"
#include "environment/geometry/elements/Patch.h"
#include "scene/VoxelGrid.h"
#include "raycasting/stochasticRaytracing/Link.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

/**
Generic routine for Stochastic Jacobi iterations:
- nr_rays: nr of rays to use
- getRadianceCallBack: routine returning radiance (total or un-shot) to be
propagated for a given element, or NULL if not radiance propagation is
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
class StochasticJacobi{ public:
    typedef ColorRgb *(*GetRadianceCallback)(const StochasticRadiosityElement *);
    typedef float (*GetImportanceCallback)(const StochasticRadiosityElement *);
    typedef void (*UpdateCallback)(StochasticRadiosityElement *elem, double w);

    static void doStochasticJacobiIteration( VoxelGrid *sceneWorldVoxelGrid, long numberOfRays, GetRadianceCallback getRadianceCallBack, GetImportanceCallback getImportanceCallBack, UpdateCallback updateCallBack, const ArrayList<Patch *> *scenePatches, RenderOptions *renderOptions);

  private:
    static GetRadianceCallback getRadianceCallback;
    static GetImportanceCallback getImportanceCallback;
    static UpdateCallback reflectCallback;
    static int useControlVariate;
    static int numberOfRaysToShoot;
    static double sumOfProbabilities;

    static void stochasticJacobiInitGlobals( int numberOfRays, GetRadianceCallback getRadianceCallBack, GetImportanceCallback getImportanceCallBack, UpdateCallback updateCallBack);
    static void stochasticJacobiPrintMessage(long numberOfRays);
    static double stochasticJacobiProbability(const StochasticRadiosityElement *elem);
    static void stchsJacElemClearAccums(StochasticRadiosityElement *elem);
    static void stochasticJacobiElementSetup(Element *element);
    static bool stochasticJacobiSetup(const ArrayList<Patch *> *scenePatches);
    static ColorRgb stchsJacGetSrcRadn(const StochasticRadiosityElement *src, double us, double vs);
    static void stchsJacPropRadnTSurf( StochasticRadiosityElement *rcv, double ur, double vr, ColorRgb rayPower, const StochasticRadiosityElement *src, double fraction, double weight);
    static void stchsJacPropRadnTClustIstrp( StochasticRadiosityElement *cluster, ColorRgb rayPower, const StochasticRadiosityElement *src, double fraction, double weight);
    static void stchsJacPropRadnClustRec( StochasticRadiosityElement *currentElement, ColorRgb rayPower, Ray *ray, float dir, double projectedArea, double fraction);
    static void stchsJacPropRadnTClustOrntd( StochasticRadiosityElement *cluster, ColorRgb rayPower, Ray *ray, float dir, const StochasticRadiosityElement *src, double projectedArea, double fraction, double weight);
    static void stchsJacPropRadn( const StochasticRadiosityElement *src, double us, double vs, StochasticRadiosityElement *rcv, double ur, double vr, double src_prob, double rcv_prob, Ray *ray, float dir);
    static void stchsJacPropImp( const StochasticRadiosityElement *src, double us, double vs, StochasticRadiosityElement *rcv, double ur, double vr, double src_prob, double rcv_prob, const Ray *ray, float dir);
    static double stchsJacRecvPrjctArea( const StochasticRadiosityElement *cluster, Ray *ray, float dir);
    static void stchsJacRecvPrjctAreaRec( const StochasticRadiosityElement *currentElement, Ray *ray, float dir, double *area);
    static void stchsJacRfnAPropRadn( const StochasticRadiosityElement *src, double us, double vs, StochasticRadiosityElement *P, double up, double vp, StochasticRadiosityElement *Q, double uq, double vq, double src_prob, double rcv_prob, Ray *ray, float dir, const RenderOptions *renderOptions);
    static void stchsJacRfnAPropImp( const StochasticRadiosityElement *P, double up, double vp, StochasticRadiosityElement *Q, double uq, double vq, double src_prob, double rcv_prob, const Ray *ray, float dir);
    static void stchsJacRfnAProp( StochasticRadiosityElement *P, double up, double vp, StochasticRadiosityElement *Q, double uq, double vq, Ray *ray, const RenderOptions *renderOptions);
    static double *stochasticJacobiNextSample( StochasticRadiosityElement *elem, int nMostSignificantBit, NiederreiterIndex mostSignificantBit1, NiederreiterIndex rMostSignificantBit2, double *zeta);
    static void stchsJacUnfrmHitCrdnt(const RayHit *hit, double *uHit, double *vHit);
    static void stochasticJacobiElementShootRay( const VoxelGrid *sceneWorldVoxelGrid, StochasticRadiosityElement *src, int nMostSignificantBit, NiederreiterIndex mostSignificantBit1, NiederreiterIndex rMostSignificantBit2, const RenderOptions *renderOptions);
    static void stchsJacElemShtRays( const VoxelGrid *sceneWorldVoxelGrid, StochasticRadiosityElement *element, int raysThisElem, const RenderOptions *renderOptions);
    static void stchsJacShtRaysRec( VoxelGrid *sceneWorldVoxelGrid, StochasticRadiosityElement *element, double rnd, long *rayCount, double *cumulative, RenderOptions *renderOptions);
    static void stochasticJacobiShootRays( VoxelGrid *sceneWorldVoxelGrid, const ArrayList<Patch *> *scenePatches, RenderOptions *renderOptions);
    static void stochasticJacobiUpdateElement(StochasticRadiosityElement *elem);
    static void stochasticJacobiPush(const StochasticRadiosityElement *parent, StochasticRadiosityElement *child);
    static void stochasticJacobiPull(StochasticRadiosityElement *parent, const StochasticRadiosityElement *child);
    static void stchsJacPushUpdPullChld(Element *element);
    static void stochasticJacobiPushUpdatePull(Element *element);
    static void stochasticJacobiClearElement(StochasticRadiosityElement *parent);
    static void stchsJacPullRdEdFromChld(Element *element);
    static void stochasticJacobiPullRdEd(StochasticRadiosityElement *element);
    static void stchsJacPushUpdPullSwp();
    static void stchsJacInitPushRayIndx(Element *element);
};

#endif

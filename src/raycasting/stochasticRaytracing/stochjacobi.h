#ifndef __STOCHASTIC_JACOBI__
#define __STOCHASTIC_JACOBI__

#include "java/util/ArrayList.h"

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
extern void
doStochasticJacobiIteration(
    VoxelGrid *sceneWorldVoxelGrid,
    long numberOfRays,
    ColorRgb *(*getRadianceCallBack)(const StochasticRadiosityElement *),
    float (*getImportanceCallBack)(const StochasticRadiosityElement *),
    void updateCallBack(StochasticRadiosityElement *elem, double w),
    const java::ArrayList<Patch *> *scenePatches,
    RenderOptions *renderOptions);

#endif

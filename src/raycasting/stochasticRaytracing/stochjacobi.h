#ifndef _STOCHJACOBI_H_
#define _STOCHJACOBI_H_

#include "java/util/ArrayList.h"

/**
Generic routine for Stochastic Jacobi iterations:
- nr_rays: nr of rays to use
- GetRadiance: routine returning radiance (total or unshot) to be
propagated for a given element, or nullptr if not radiance propagation is
required.
- GetImportance: same, but for importance.
- Update: routine updating total, unshot and source radiance and/or
importance based on result received during the iteration.

The operation of this routine is further controlled by global parameters
- GLOBAL_stochasticRaytracing_monteCarloRadiosityState.do_control_radiosity: perform constant control variate variance reduction
- GLOBAL_stochasticRaytracing_monteCarloRadiosityState.bidirectionalTransfers: for using lines bidirectionally
- GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven: importance-driven radiance propagation
- GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven: radiance-driven importance propagation
- hierarchy.do_h_meshing, hierarchy.clustering: hierarchical refinement/clustering

This routine updates global ray counts and total/unshot power/importance statistics.

CAVEAT: propagate either radiance or importance alone. Simultaneous
propagation of importance and radiance does not work yet.
*/
extern void doStochasticJacobiIteration(
    long nr_rays,
    COLOR *(*GetRadiance)(StochasticRadiosityElement *),
    float (*GetImportance)(StochasticRadiosityElement *),
    void Update(StochasticRadiosityElement *elem, double w),
    java::ArrayList<Patch *> *scenePatches);

#endif

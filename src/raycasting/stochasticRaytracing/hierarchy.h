/**
Hierarchical refinement stuff (includes Jan's elementP.h)
*/

#ifndef __ELEMENT_HIERARCHY__
#define __ELEMENT_HIERARCHY__

#include "java/util/ArrayList.h"

#include "raycasting/stochasticRaytracing/LINK.h"
#include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "raycasting/stochasticRaytracing/HierarchyClusteringMode.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

extern REFINE_ACTION powerOracle(const LINK *link);
extern LINK topLink(StochasticRadiosityElement *rcvTop, StochasticRadiosityElement *srcTop);
extern LINK *hierarchyRefine(
    LINK *link,
    StochasticRadiosityElement *rcvTop,
    double *ur,
    double *vr,
    StochasticRadiosityElement *srcTop,
    double *us,
    double *vs,
    ORACLE evaluateLink,
    const RenderOptions *renderOptions);

extern ElementHierarchyState GLOBAL_stochasticRaytracing_hierarchy;

extern void elementHierarchyDefaults();
extern void elementHierarchyInit(Geometry *clusteredWorldGeometry);
extern void elementHierarchyTerminate(const java::ArrayList<Patch *> *scenePatches);

#endif

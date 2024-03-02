#ifndef __GALERKIN_ELEMENT__
#define __GALERKIN_ELEMENT__

/**
Data associated with each Patch:
*/
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

/**
Maximum element hierarchy depth: logFatal errors in the macros below if
the element hierarchy is deeper than this, but that is surely not a
normal situation anyways!
*/
#define MAX_HIERARCHY_DEPTH 100

#include "common/dataStructures/stackmac.h"

extern Matrix2x2 GLOBAL_stochasticRaytracing_quadUpTransform[4];
extern Matrix2x2 GLOBAL_stochasticRaytracing_triangleUpTransform[4];

extern StochasticRadiosityElement *monteCarloRadiosityCreateToplevelSurfaceElement(Patch *patch);
extern void monteCarloRadiosityDestroyToplevelSurfaceElement(StochasticRadiosityElement *elem);
extern StochasticRadiosityElement *monteCarloRadiosityCreateClusterHierarchy(Geometry *world);
extern void monteCarloRadiosityDestroyClusterHierarchy(StochasticRadiosityElement *top);
extern void monteCarloRadiosityPrintElement(FILE *out, StochasticRadiosityElement *elem);
extern void monteCarloRadiosityForAllLeafElements(StochasticRadiosityElement *top, void (*func)(StochasticRadiosityElement *));
extern void monteCarloRadiosityForAllSurfaceLeafs(StochasticRadiosityElement *top, void (*func)(StochasticRadiosityElement *));
extern int monteCarloRadiosityForAllChildrenElements(StochasticRadiosityElement *top, void (*func)(StochasticRadiosityElement *));
extern int monteCarloRadiosityElementIsLeaf(StochasticRadiosityElement *elem);
extern void monteCarloRadiosityElementRange(StochasticRadiosityElement *elem, int *nbits, niedindex *msb1, niedindex *rmsb2);
extern float *monteCarloRadiosityElementBounds(StochasticRadiosityElement *elem, float *bounds);
extern StochasticRadiosityElement *monteCarloRadiosityClusterChildContainingElement(StochasticRadiosityElement *parent, StochasticRadiosityElement *descendant);
extern StochasticRadiosityElement **monteCarloRadiosityRegularSubdivideElement(StochasticRadiosityElement *element);
extern StochasticRadiosityElement *monteCarloRadiosityRegularSubElementAtPoint(StochasticRadiosityElement *parent, double *u, double *v);
extern StochasticRadiosityElement *monteCarloRadiosityRegularLeafElementAtPoint(StochasticRadiosityElement *top, double *u, double *v);
extern Vertex *monteCarloRadiosityEdgeMidpointVertex(StochasticRadiosityElement *elem, int edgeNumber);
extern int monteCarloRadiosityElementIsTextured(StochasticRadiosityElement *elem);
extern float monteCarloRadiosityElementScalarReflectance(StochasticRadiosityElement *elem);
extern void pushRadiance(StochasticRadiosityElement *parent, StochasticRadiosityElement *child, COLOR *parent_rad, COLOR *child_rad);
extern void pushImportance(StochasticRadiosityElement *parent, StochasticRadiosityElement *child, float *parent_imp, float *child_imp);
extern void pullRadiance(StochasticRadiosityElement *parent, StochasticRadiosityElement *child, COLOR *parent_rad, COLOR *child_rad);
extern void pullImportance(StochasticRadiosityElement *parent, StochasticRadiosityElement *child, float *parent_imp, float *child_imp);
extern COLOR elementDisplayRadiance(StochasticRadiosityElement *elem);
extern COLOR elementDisplayRadianceAtPoint(StochasticRadiosityElement *elem, double u, double v);
extern void mcrRenderElement(StochasticRadiosityElement *elem);
extern void renderElementOutline(StochasticRadiosityElement *elem);
extern void elementComputeNewVertexColors(StochasticRadiosityElement *elem);
extern void elementAdjustTVertexColors(StochasticRadiosityElement *elem);
extern RGB elementColor(StochasticRadiosityElement *element);
extern COLOR vertexReflectance(Vertex *v);

extern void
elementTVertexElimination(
    StochasticRadiosityElement *elem,
    void (*do_triangle)(Vertex *, Vertex *, Vertex *),
    void (*do_quadrilateral)(Vertex *, Vertex *, Vertex *, Vertex *));

/**
Iterates over all leaf elements in the surface element hierarchy with
'top' (a surface element) on top
*/
#define REC_ForAllSurfaceLeafs(leaf, top) { \
    int _i_ = -1; \
    int _did_recurse = false; \
    STACK_DECL(int, _isave, MAX_HIERARCHY_DEPTH, _isaveptr); \
    StochasticRadiosityElement *_curel = (top); \
  _begin_recurse_SL: \
    if ( _curel->regularSubElements ) { /* recursive case */ \
        _did_recurse = true; \
        STACK_SAVE(_i_, _isave, MAX_HIERARCHY_DEPTH, _isaveptr); \
        for ( _i_ = 0; _i_ < 4; _i_++ ) { \
            _curel = _curel->regularSubElements[_i_]; \
            goto _begin_recurse_SL; \
          _end_recurse_SL: \
            _curel = _curel->parent; \
        } \
        STACK_RESTORE_NOCHECK(_i_, _isave, _isaveptr); \
        if ( _curel != (top) ) {/* not back at top */ \
            goto _end_recurse_SL; \
        } \
    } else { \
        StochasticRadiosityElement *(leaf) = _curel;

/**
Do something with 'leaf'
*/
#define REC_EndForAllSurfaceLeafs \
    if (_did_recurse) \
      goto _end_recurse_SL; \
  } \
}

#endif

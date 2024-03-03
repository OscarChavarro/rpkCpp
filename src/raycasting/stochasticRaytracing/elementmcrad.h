#ifndef __GALERKIN_ELEMENT__
#define __GALERKIN_ELEMENT__

#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

extern Matrix2x2 GLOBAL_stochasticRaytracing_quadUpTransform[4];
extern Matrix2x2 GLOBAL_stochasticRaytracing_triangleUpTransform[4];

extern StochasticRadiosityElement *monteCarloRadiosityCreateToplevelSurfaceElement(Patch *patch);
extern void monteCarloRadiosityDestroyToplevelSurfaceElement(StochasticRadiosityElement *elem);
extern StochasticRadiosityElement *monteCarloRadiosityCreateClusterHierarchy(Geometry *world);
extern void monteCarloRadiosityDestroyClusterHierarchy(StochasticRadiosityElement *top);
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

#endif

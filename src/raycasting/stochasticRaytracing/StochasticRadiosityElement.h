/**
Monte Carlo radiosity element type
*/

#ifndef __STOCHASTIC_RADIOSITY_ELEMENT__
#define __STOCHASTIC_RADIOSITY_ELEMENT__

#include "java/util/ArrayList.h"
#include "skin/Element.h"
#include "QMC/niederreiter.h"
#include "raycasting/stochasticRaytracing/basismcrad.h"

class StochasticRadiosityElement : public Element {
  public:
    niedindex rayIndex; // Incremented each time a ray is shot from the element
    float quality; // For merging the result of multiple iterations
    float samplingProbability;
    float ng; // Number of samples gathered on the patch

    GalerkinBasis *basis; // Radiosity approximation data
        // Higher order approximations need an array of color values for representing radiance
    COLOR sourceRad; // Always constant source radiosity

    float importance; // For view-importance driven sampling
    float unShotImportance;
    float receivedImportance;
    float sourceImportance;
    niedindex importanceRayIndex; // Ray index for importance propagation

    Vector3D midPoint;
    Vertex *vertices[4]; // Up to 4 vertex pointers for surface elements
    signed char childNumber; // -1 for clusters or toplevel surface elements, 0..3 for regular surface sub-elements
    char numberOfVertices; // Number of surface element vertices

    StochasticRadiosityElement();
    ~StochasticRadiosityElement();
};

extern Matrix2x2 GLOBAL_stochasticRaytracing_quadUpTransform[4];
extern Matrix2x2 GLOBAL_stochasticRaytracing_triangleUpTransform[4];

// Constructors
extern StochasticRadiosityElement *stochasticRadiosityElementCreateFromPatch(Patch *patch);
extern StochasticRadiosityElement *stochasticRadiosityElementCreateFromGeometry(Geometry *world);

// Destructors
extern void stochasticRadiosityElementDestroy(StochasticRadiosityElement *elem);
extern void stochasticRadiosityElementDestroyClusterHierarchy(StochasticRadiosityElement *top);

// Traversals
extern void stochasticRadiosityElementTraverseLeafElements(StochasticRadiosityElement *top, void (*traversalCallbackFunction)(StochasticRadiosityElement *));
extern void stochasticRadiosityElementTraverseSurfaceLeafs(StochasticRadiosityElement *top, void (*func)(StochasticRadiosityElement *));
extern int stochasticRadiosityElementTraverseChildrenElements(StochasticRadiosityElement *top, void (*func)(StochasticRadiosityElement *));

// Standard methods
extern void
stochasticRadiosityElementRange(
    StochasticRadiosityElement *elem,
    int *numberOfBits,
    niedindex *mostSignificantBits1,
    niedindex *rMostSignificantBits2);
extern float *stochasticRadiosityElementBounds(StochasticRadiosityElement *elem, float *bounds);
extern StochasticRadiosityElement **stochasticRadiosityElementRegularSubdivideElement(StochasticRadiosityElement *element);
extern StochasticRadiosityElement *stochasticRadiosityElementRegularSubElementAtPoint(StochasticRadiosityElement *parent, double *u, double *v);
extern StochasticRadiosityElement *stochasticRadiosityElementRegularLeafElementAtPoint(StochasticRadiosityElement *top, double *u, double *v);
extern Vertex *stochasticRadiosityElementEdgeMidpointVertex(StochasticRadiosityElement *elem, int edgeNumber);
extern int stochasticRadiosityElementIsTextured(StochasticRadiosityElement *elem);
extern float stochasticRadiosityElementScalarReflectance(StochasticRadiosityElement *elem);
extern void stochasticRadiosityElementPushRadiance(StochasticRadiosityElement *parent, StochasticRadiosityElement *child, COLOR *parent_rad, COLOR *child_rad);
extern void stochasticRadiosityElementPushImportance(const float *parentImportance, float *childImportance);
extern void stochasticRadiosityElementPullRadiance(StochasticRadiosityElement *parent, StochasticRadiosityElement *child, COLOR *parent_rad, COLOR *child_rad);
extern void stochasticRadiosityElementPullImportance(StochasticRadiosityElement *parent, StochasticRadiosityElement *child, float *parent_imp, const float *child_imp);

// In render.cpp
extern COLOR stochasticRadiosityElementDisplayRadiance(StochasticRadiosityElement *elem);
extern COLOR stochasticRadiosityElementDisplayRadianceAtPoint(StochasticRadiosityElement *elem, double u, double v);
extern void stochasticRadiosityElementRender(StochasticRadiosityElement *elem);
extern void stochasticRadiosityElementRenderOutline(StochasticRadiosityElement *elem);
extern void stochasticRadiosityElementComputeNewVertexColors(StochasticRadiosityElement *elem);
extern void stochasticRadiosityElementAdjustTVertexColors(StochasticRadiosityElement *elem);
extern RGB stochasticRadiosityElementColor(StochasticRadiosityElement *element);

extern void
stochasticRadiosityElementTVertexElimination(
        StochasticRadiosityElement *elem,
        void (*do_triangle)(Vertex *, Vertex *, Vertex *),
        void (*do_quadrilateral)(Vertex *, Vertex *, Vertex *, Vertex *));

#endif

/**
Monte Carlo radiosity element type
*/

#ifndef __STOCHASTIC_RADIOSITY_ELEMENT__
#define __STOCHASTIC_RADIOSITY_ELEMENT__

#include "java/util/ArrayList.h"
#include "skin/Element.h"
#include "common/quasiMonteCarlo/Niederreiter.h"
#include "raycasting/stochasticRaytracing/basismcrad.h"

class StochasticRadiosityElement : public Element {
  public:
    NiederreiterIndex rayIndex; // Incremented each time a ray is shot from the element
    float quality; // For merging the result of multiple iterations
    float samplingProbability;
    float ng; // Number of samples gathered on the patch

    GalerkinBasis *basis; // Radiosity approximation data
        // Higher order approximations need an array of color values for representing radiance
    ColorRgb sourceRad; // Always constant source radiosity

    float importance; // For view-importance driven sampling
    float unShotImportance;
    float receivedImportance;
    float sourceImportance;
    NiederreiterIndex importanceRayIndex; // Ray index for importance propagation

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

// Standard methods
extern void
stochasticRadiosityElementRange(
        StochasticRadiosityElement *elem,
        int *numberOfBits,
        NiederreiterIndex *mostSignificantBits1,
        NiederreiterIndex *rMostSignificantBits2);

extern float *
stochasticRadiosityElementBounds(StochasticRadiosityElement *elem, BoundingBox *boundingBox);

extern StochasticRadiosityElement **
stochasticRadiosityElementRegularSubdivideElement(
    StochasticRadiosityElement *element, const RenderOptions *renderOptions);

extern StochasticRadiosityElement *
stochasticRadiosityElementRegularSubElementAtPoint(
    const StochasticRadiosityElement *parent, double *u, double *v);

extern StochasticRadiosityElement *
stochasticRadiosityElementRegularLeafElementAtPoint(
    StochasticRadiosityElement *top, double *u, double *v);

extern Vertex *
stochasticRadiosityElementEdgeMidpointVertex(
    const StochasticRadiosityElement *elem, int edgeNumber);

extern int stochasticRadiosityElementIsTextured(const StochasticRadiosityElement *elem);
extern float stochasticRadiosityElementScalarReflectance(const StochasticRadiosityElement *elem);

extern void
stochasticRadiosityElementPushRadiance(
    const StochasticRadiosityElement *parent,
    StochasticRadiosityElement *child,
    ColorRgb *parentRadiance,
    ColorRgb *childRadiance);

extern void stochasticRadiosityElementPushImportance(const float *parentImportance, float *childImportance);

extern void
stochasticRadiosityElementPullRadiance(
    const StochasticRadiosityElement *parent,
    const StochasticRadiosityElement *child,
    ColorRgb *parent_rad,
    ColorRgb *child_rad);

extern void
stochasticRadiosityElementPullImportance(
    const StochasticRadiosityElement *parent,
    const StochasticRadiosityElement *child,
    float *parent_imp,
    const float *child_imp);

// In render.cpp
extern ColorRgb
stochasticRadiosityElementDisplayRadiance(StochasticRadiosityElement *elem);

extern ColorRgb
stochasticRadiosityElementDisplayRadianceAtPoint(
    StochasticRadiosityElement *elem, double u, double v, const RenderOptions *renderOptions);

extern void
stochasticRadiosityElementRender(Element *element, const RenderOptions *renderOptions);

extern void
stochasticRadiosityElementComputeNewVertexColors(Element *element);

extern void
stochasticRadiosityElementAdjustTVertexColors(Element *element);

extern ColorRgb
stochasticRadiosityElementColor(StochasticRadiosityElement *element);

#endif

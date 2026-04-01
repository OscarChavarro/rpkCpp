/**
Monte Carlo radiosity element type
*/

#ifndef __STOCHASTIC_RADIOSITY_ELEMENT__
#define __STOCHASTIC_RADIOSITY_ELEMENT__

#include "java/util/ArrayList.h"
#include "numericalAnalysis/quasiMonteCarlo/Niederreiter.h"
#include "skin/Element.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"

class StochasticRadiosityElement final : public Element {
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

    static StochasticRadiosityElement *stochasticRadiosityElementCreateFromPatch(Patch *patch);
    static StochasticRadiosityElement *stochasticRadiosityElementCreateFromGeometry(Geometry *world);

    static void stochasticRadiosityElementDestroy(StochasticRadiosityElement *elem);
    static void stochasticRadiosityElementDestroyClusterHierarchy(StochasticRadiosityElement *top);

    static void stochasticRadiosityElementRange(
        StochasticRadiosityElement *elem,
        int *numberOfBits,
        NiederreiterIndex *mostSignificantBits1,
        NiederreiterIndex *mostSignificantBits2);
    static StochasticRadiosityElement **stochasticRadiosityElementRegularSubdivideElement(
        StochasticRadiosityElement *element,
        const RenderOptions *renderOptions);
    static StochasticRadiosityElement *stochasticRadiosityElementRegularSubElementAtPoint(
        const StochasticRadiosityElement *parent,
        double *u,
        double *v);
    static StochasticRadiosityElement *stochasticRadiosityElementRegularLeafElementAtPoint(
        StochasticRadiosityElement *top,
        double *u,
        double *v);
    static Vertex *stochasticRadiosityElementEdgeMidpointVertex(const StochasticRadiosityElement *elem, int edgeNumber);

    static int stochasticRadiosityElementIsTextured(const StochasticRadiosityElement *elem);
    static float stochasticRadiosityElementScalarReflectance(const StochasticRadiosityElement *elem);
    static void stochasticRadiosityElementPushRadiance(
        const StochasticRadiosityElement *parent,
        StochasticRadiosityElement *child,
        const ColorRgb *parentRadiance,
        ColorRgb *childRadiance);
    static void stochasticRadiosityElementPushImportance(const float *parentImportance, float *childImportance);
    static void stochasticRadiosityElementPullRadiance(
        const StochasticRadiosityElement *parent,
        const StochasticRadiosityElement *child,
        ColorRgb *parentRad,
        const ColorRgb *childRad);
    static void stochasticRadiosityElementPullImportance(
        const StochasticRadiosityElement *parent,
        const StochasticRadiosityElement *child,
        float *parentImportance,
        const float *childImportance);

    static ColorRgb stochasticRadiosityElementDisplayRadiance(const StochasticRadiosityElement *elem);
    static ColorRgb stochasticRadiosityElementDisplayRadianceAtPoint(
        const StochasticRadiosityElement *elem,
        double u,
        double v,
        const RenderOptions *renderOptions);
    static void stochasticRadiosityElementComputeNewVertexColors(Element *element);
    static void stochasticRadiosityElementAdjustTVertexColors(Element *element);
    static ColorRgb stochasticRadiosityElementColor(const StochasticRadiosityElement *element);

    StochasticRadiosityElement();
    ~StochasticRadiosityElement() final;

  private:
    static void vertexAttachElement(Vertex *vertex, StochasticRadiosityElement *elem);
    static StochasticRadiosityElement *createElement();
    static StochasticRadiosityElement *monteCarloRadiosityCreateCluster(Geometry *geometry);
    static void monteCarloRadiosityCreateSurfaceElementChild(Patch *patch, StochasticRadiosityElement *parent);
    static void monteCarloRadiosityCreateClusterChild(Geometry *geometry, StochasticRadiosityElement *parent);
    static void monteCarloRadiosityInitClusterPull(StochasticRadiosityElement *parent, const StochasticRadiosityElement *child);
    static void monteCarloRadiosityCreateClusterChildren(StochasticRadiosityElement *parent);
    static StochasticRadiosityElement *monteCarloRadiosityCreateClusterHierarchyRecursive(Geometry *world);
    static Vector3D galerkinElementMidpoint(StochasticRadiosityElement *elem);
    static Vector3D *monteCarloRadiosityInstallCoordinate(const Vector3D *coord);
    static Vector3D *monteCarloRadiosityInstallNormal(const Vector3D *normal);
    static Vector3D *monteCarloRadiosityInstallTexCoord(const Vector3D *texCoord);
    static Vertex *monteCarloRadiosityInstallVertex(Vector3D *coord, Vector3D *normal, Vector3D *texCoord);
    static Vertex *monteCarloRadiosityNewMidpointVertex(
        StochasticRadiosityElement *elem,
        const Vertex *vertex1,
        const Vertex *vertex2);
    static StochasticRadiosityElement *monteCarloRadiosityElementNeighbour(
        const StochasticRadiosityElement *elem,
        int edgeNumber);
    static Vertex *monteCarloRadiosityNewEdgeMidpointVertex(StochasticRadiosityElement *elem, int edgeNumber);
    static void monteCarloRadiosityElementComputeAverageReflectanceAndEmittance(StochasticRadiosityElement *elem);
    static void monteCarloRadiosityInitSurfacePush(const StochasticRadiosityElement *parent, StochasticRadiosityElement *child);
    static StochasticRadiosityElement *monteCarloRadiosityCreateSurfaceSubElement(
        StochasticRadiosityElement *parent,
        int childNumber,
        Vertex *v0,
        Vertex *v1,
        Vertex *v2,
        Vertex *v3);
    static StochasticRadiosityElement **monteCarloRadiosityRegularSubdivideTriangle(
        StochasticRadiosityElement *element,
        const RenderOptions *renderOptions);
    static StochasticRadiosityElement **monteCarloRadiosityRegularSubdivideQuad(
        StochasticRadiosityElement *element,
        const RenderOptions *renderOptions);
    static void monteCarloRadiosityDestroyElement(StochasticRadiosityElement *elem);
    static void monteCarloRadiosityDestroySurfaceElement(StochasticRadiosityElement *elem);
    static bool regularChild(const StochasticRadiosityElement *child);
    static ColorRgb vertexRadiance(const Vertex *vertex);
    static float vertexImportance(const Vertex *vertex);
    static ColorRgb vertexColor(Vertex *vertex);
};

extern Matrix2x2 GLOBAL_stochasticRaytracing_quadUpTransform[4];
extern Matrix2x2 GLOBAL_stochasticRaytracing_triangleUpTransform[4];

#endif

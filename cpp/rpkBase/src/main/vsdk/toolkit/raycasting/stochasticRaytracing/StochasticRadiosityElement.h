/**
Monte Carlo radiosity element type
*/

#ifndef STOCHASTIC_RADIOSITY_ELEMENT__
#define STOCHASTIC_RADIOSITY_ELEMENT__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/numericalAnalysis/quasiMonteCarlo/Niederreiter.h"
#include "vsdk/toolkit/environment/geometry/elements/Element.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Basismcrad.h"

class Coefficientsmcrad;

class StochasticRadiosityElement final : public Element {
  public:
    Patch *patch;
    RpkGeometry *geometry;
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

    Vector3Df midPoint;
    RpkVertex *vertices[4]; // Up to 4 vertex pointers for surface elements
    signed char childNumber; // -1 for clusters or toplevel surface elements, 0..3 for regular surface sub-elements
    char numberOfVertices; // Number of surface element vertices

    static StochasticRadiosityElement *stochasticRadiosityElementCreateFromPatch(Patch *patch);
    static StochasticRadiosityElement *stochasticRadiosityElementCreateFromGeometry(RpkGeometry *world);

    static void stochasticRadiosityElementDestroy(StochasticRadiosityElement *elem);
    static void stochasticRadiosityElementDestroyClusterHierarchy(StochasticRadiosityElement *top);

    static void stochasticRadiosityElementRange(
        StochasticRadiosityElement *elem,
        int *numberOfBits,
        NiederreiterIndex *mostSignificantBits1,
        NiederreiterIndex *mostSignificantBits2);
    static StochasticRadiosityElement **stochasticRadiosityElementRegularSubdivideElement(
        StochasticRadiosityElement *element,
        const RendererConfiguration *renderOptions);
    static StochasticRadiosityElement *stochasticRadiosityElementRegularSubElementAtPoint(
        const StochasticRadiosityElement *parent,
        double *u,
        double *v);
    static StochasticRadiosityElement *stochasticRadiosityElementRegularLeafElementAtPoint(
        StochasticRadiosityElement *top,
        double *u,
        double *v);
    static RpkVertex *stochasticRadiosityElementEdgeMidpointVertex(const StochasticRadiosityElement *elem, int edgeNumber);

    static bool stochasticRadiosityElementIsTextured(const StochasticRadiosityElement *elem);
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
        const RendererConfiguration *renderOptions);
    static void stochasticRadiosityElementComputeNewVertexColors(Element *element);
    static void stochasticRadiosityElementAdjustTVertexColors(Element *element);
    static ColorRgb stochasticRadiosityElementColor(const StochasticRadiosityElement *element);
    static bool coefficientPoolsAreInitialized();
    static void markCoefficientPoolsInitialized();

    StochasticRadiosityElement();
    ~StochasticRadiosityElement() final;

    Patch *getPatch() const {
        return patch;
    }

    void setPatch(Patch *inPatch) {
        patch = inPatch;
    }

    RpkGeometry *getGeometry() const {
        return geometry;
    }

    void setGeometry(RpkGeometry *inGeometry) {
        geometry = inGeometry;
    }

  private:
    static int coefficientPoolsInitialized;

    static void vertexAttachElement(RpkVertex *vertex, StochasticRadiosityElement *elem);
    static StochasticRadiosityElement *createElement();
    static StochasticRadiosityElement *monteCarloRadiosityCreateCluster(RpkGeometry *geometry);
    static void monteCarloRadiosityCreateSurfaceElementChild(Patch *patch, StochasticRadiosityElement *parent);
    static void monteCarloRadiosityCreateClusterChild(RpkGeometry *geometry, StochasticRadiosityElement *parent);
    static void monteCarloRadiosityInitClusterPull(StochasticRadiosityElement *parent, const StochasticRadiosityElement *child);
    static void monteCarloRadiosityCreateClusterChildren(StochasticRadiosityElement *parent);
    static StochasticRadiosityElement *monteCarloRadiosityCreateClusterHierarchyRecursive(RpkGeometry *world);
    static Vector3Df galerkinElementMidpoint(StochasticRadiosityElement *elem);
    static Vector3Df *monteCarloRadiosityInstallCoordinate(const Vector3Df *coord);
    static Vector3Df *monteCarloRadiosityInstallNormal(const Vector3Df *normal);
    static Vector3Df *monteCarloRadiosityInstallTexCoord(const Vector3Df *texCoord);
    static RpkVertex *monteCarloRadiosityInstallVertex(Vector3Df *coord, Vector3Df *normal, Vector3Df *texCoord);
    static RpkVertex *monteCarloRadiosityNewMidpointVertex(
        StochasticRadiosityElement *elem,
        const RpkVertex *vertex1,
        const RpkVertex *vertex2);
    static StochasticRadiosityElement *monteCarloRadiosityElementNeighbour(
        const StochasticRadiosityElement *elem,
        int edgeNumber);
    static RpkVertex *monteCarloRadiosityNewEdgeMidpointVertex(StochasticRadiosityElement *elem, int edgeNumber);
    static void monteCarloRadiosityElementComputeAverageReflectanceAndEmittance(StochasticRadiosityElement *elem);
    static void monteCarloRadiosityInitSurfacePush(const StochasticRadiosityElement *parent, StochasticRadiosityElement *child);
    static StochasticRadiosityElement *monteCarloRadiosityCreateSurfaceSubElement(
        StochasticRadiosityElement *parent,
        int childNumber,
        RpkVertex *v0,
        RpkVertex *v1,
        RpkVertex *v2,
        RpkVertex *v3);
    static StochasticRadiosityElement **monteCarloRadiosityRegularSubdivideTriangle(
        StochasticRadiosityElement *element,
        const RendererConfiguration *renderOptions);
    static StochasticRadiosityElement **monteCarloRadiosityRegularSubdivideQuad(
        StochasticRadiosityElement *element,
        const RendererConfiguration *renderOptions);
    static void monteCarloRadiosityDestroyElement(StochasticRadiosityElement *elem);
    static void monteCarloRadiosityDestroySurfaceElement(StochasticRadiosityElement *elem);
    static bool regularChild(const StochasticRadiosityElement *child);
    static ColorRgb vertexRadiance(const RpkVertex *vertex);
    static float vertexImportance(const RpkVertex *vertex);
    static ColorRgb vertexColor(RpkVertex *vertex);
};

#endif

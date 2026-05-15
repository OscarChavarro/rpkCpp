/**
Monte Carlo radiosity element type
*/

#ifndef STCHS_RDSTY_ELMNT
#define STCHS_RDSTY_ELMNT

#include "java/util/ArrayList.h"
#include "numericalAnalysis/quasiMonteCarlo/Niederreiter.h"
#include "environment/geometry/elements/Element.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"

class Coefficientsmcrad;

class StochasticRadiosityElement: public Element{ public:
    Patch *patch;
    Geometry *geometry;
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

    static StochasticRadiosityElement *stchsRadElemCreateFromPtch(Patch *patch);
    static StochasticRadiosityElement *stchsRadElemCreateFromGeom(Geometry *world);

    static void stchsRadElemDestroy(StochasticRadiosityElement *elem);
    static void stchsRadElemDestroyClustHier(StochasticRadiosityElement *top);

    static void stochasticRadiosityElementRange( StochasticRadiosityElement *elem, int *numberOfBits, NiederreiterIndex *mostSignificantBits1, NiederreiterIndex *mostSignificantBits2);
    static StochasticRadiosityElement **stchsRadElemRegSbdvdElem( StochasticRadiosityElement *element, const RenderOptions *renderOptions);
    static StochasticRadiosityElement *stchsRadElemRegSubElemAPnt( const StochasticRadiosityElement *parent, double *u, double *v);
    static StochasticRadiosityElement *stchsRadElemRegLeafElemAPnt( StochasticRadiosityElement *top, double *u, double *v);
    static Vertex *stchsRadElemEdgeMidVtx(const StochasticRadiosityElement *elem, int edgeNumber);

    static bool stchsRadElemITex(const StochasticRadiosityElement *elem);
    static float stchsRadElemSclrRefl(const StochasticRadiosityElement *elem);
    static void stchsRadElemPushRadn( const StochasticRadiosityElement *parent, StochasticRadiosityElement *child, const ColorRgbMutable *parentRadiance, ColorRgbMutable *childRadiance);
    static void stchsRadElemPushImp(const float *parentImportance, float *childImportance);
    static void stchsRadElemPullRadn( const StochasticRadiosityElement *parent, const StochasticRadiosityElement *child, ColorRgbMutable *parentRad, const ColorRgbMutable *childRad);
    static void stchsRadElemPullImp( const StochasticRadiosityElement *parent, const StochasticRadiosityElement *child, float *parentImportance, const float *childImportance);

    static ColorRgb stchsRadElemDispRadn(const StochasticRadiosityElement *elem);
    static ColorRgb stchsRadElemDispRadnAPnt( const StochasticRadiosityElement *elem, double u, double v, const RenderOptions *renderOptions);
    static void stchsRadElemCompNewVtxClrs(Element *element);
    static void stchsRadElemAdjTVtxClrs(Element *element);
    static ColorRgb stochasticRadiosityElementColor(const StochasticRadiosityElement *element);
    static bool coefficientPoolsAreInitialized();
    static void markCoefficientPoolsInitialized();

    StochasticRadiosityElement();
    ~StochasticRadiosityElement();

    Patch *getPatch() const{ return patch;
    }

    void setPatch(Patch *inPatch){ patch = inPatch;
    }

    Geometry *getGeometry() const{ return geometry;
    }

    void setGeometry(Geometry *inGeometry){ geometry = inGeometry;
    }

  private:
    static int coefficientPoolsInitialized;

    static void vertexAttachElement(Vertex *vertex, StochasticRadiosityElement *elem);
    static StochasticRadiosityElement *createElement();
    static StochasticRadiosityElement *mntCarloRadCreateClust(Geometry *geometry);
    static void mntCarloRadCreateSurfElemChld(Patch *patch, StochasticRadiosityElement *parent);
    static void mntCarloRadCreateClustChld(Geometry *geometry, StochasticRadiosityElement *parent);
    static void mntCarloRadInitClustPull(StochasticRadiosityElement *parent, const StochasticRadiosityElement *child);
    static void mntCarloRadCreateClustChildren(StochasticRadiosityElement *parent);
    static StochasticRadiosityElement *mntCarloRadCreateClustHierRec(Geometry *world);
    static Vector3D galerkinElementMidpoint(StochasticRadiosityElement *elem);
    static Vector3D *mntCarloRadInstCoord(const Vector3D *coord);
    static Vector3D *mntCarloRadInstNorm(const Vector3D *normal);
    static Vector3D *mntCarloRadInstTexCoord(const Vector3D *texCoord);
    static Vertex *mntCarloRadInstVtx(Vector3D *coord, Vector3D *normal, Vector3D *texCoord);
    static Vertex *mntCarloRadNewMidVtx( StochasticRadiosityElement *elem, const Vertex *vertex1, const Vertex *vertex2);
    static StochasticRadiosityElement *mntCarloRadElemNghbr( const StochasticRadiosityElement *elem, int edgeNumber);
    static Vertex *mntCarloRadNewEdgeMidVtx(StochasticRadiosityElement *elem, int edgeNumber);
    static void mntCarloRadElemCompAvgReflAEmit(StochasticRadiosityElement *elem);
    static void mntCarloRadInitSurfPush(const StochasticRadiosityElement *parent, StochasticRadiosityElement *child);
    static StochasticRadiosityElement *mntCarloRadCreateSurfSubElem( StochasticRadiosityElement *parent, int childNumber, Vertex *v0, Vertex *v1, Vertex *v2, Vertex *v3);
    static StochasticRadiosityElement **mntCarloRadRegSbdvdTri( StochasticRadiosityElement *element, const RenderOptions *renderOptions);
    static StochasticRadiosityElement **mntCarloRadRegSbdvdQuad( StochasticRadiosityElement *element, const RenderOptions *renderOptions);
    static void mntCarloRadDestroyElem(StochasticRadiosityElement *elem);
    static void mntCarloRadDestroySurfElem(StochasticRadiosityElement *elem);
    static bool regularChild(const StochasticRadiosityElement *child);
    static ColorRgb vertexRadiance(const Vertex *vertex);
    static float vertexImportance(const Vertex *vertex);
    static ColorRgb vertexColor(Vertex *vertex);
};

#endif

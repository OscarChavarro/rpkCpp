/**
Monte Carlo radiosity element type
*/

#ifndef STCHS_RDSTY_ELMNT
#define STCHS_RDSTY_ELMNT

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/numericalAnalysis/quasiMonteCarlo/Niederreiter.h"
#include "vsdk/toolkit/environment/geometry/elements/Element.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Basismcrad.h"

class Coefficientsmcrad;

class StochasticRadiosityElement: public Element{ public:
    Patch *patch;
    RpkGeometry *geometry;
    NiederreiterIndex rayIndex; // Incremented each time a ray is shot from the element
    double quality; // For merging the result of multiple iterations
    double samplingProbability;
    double ng; // Number of samples gathered on the patch

    GalerkinBasis *basis; // Radiosity approximation data
        // Higher order approximations need an array of color values for representing radiance
    ColorRgb sourceRad; // Always constant source radiosity

    double importance; // For view-importance driven sampling
    double unShotImportance;
    double receivedImportance;
    double sourceImportance;
    NiederreiterIndex importanceRayIndex; // Ray index for importance propagation

    Vector3Dd midPoint;
    RpkVertex *vertices[4]; // Up to 4 vertex pointers for surface elements
    signed char childNumber; // -1 for clusters or toplevel surface elements, 0..3 for regular surface sub-elements
    char numberOfVertices; // Number of surface element vertices

    static StochasticRadiosityElement *stchsRadElemCreateFromPtch(Patch *patch);
    static StochasticRadiosityElement *stchsRadElemCreateFromGeom(RpkGeometry *world);

    static void stchsRadElemDestroy(StochasticRadiosityElement *elem);
    static void stchsRadElemDestroyClustHier(StochasticRadiosityElement *top);

    static void stochasticRadiosityElementRange( StochasticRadiosityElement *elem, int *numberOfBits, NiederreiterIndex *mostSignificantBits1, NiederreiterIndex *mostSignificantBits2);
    static StochasticRadiosityElement **stchsRadElemRegSbdvdElem( StochasticRadiosityElement *element, const RenderOptions *renderOptions);
    static StochasticRadiosityElement *stchsRadElemRegSubElemAPnt( const StochasticRadiosityElement *parent, double *u, double *v);
    static StochasticRadiosityElement *stchsRadElemRegLeafElemAPnt( StochasticRadiosityElement *top, double *u, double *v);
    static RpkVertex *stchsRadElemEdgeMidVtx(const StochasticRadiosityElement *elem, int edgeNumber);

    static bool stchsRadElemITex(const StochasticRadiosityElement *elem);
    static double stchsRadElemSclrRefl(const StochasticRadiosityElement *elem);
    static void stchsRadElemPushRadn( const StochasticRadiosityElement *parent, StochasticRadiosityElement *child, const ColorRgbMutable *parentRadiance, ColorRgbMutable *childRadiance);
    static void stchsRadElemPushImp(const double *parentImportance, double *childImportance);
    static void stchsRadElemPullRadn( const StochasticRadiosityElement *parent, const StochasticRadiosityElement *child, ColorRgbMutable *parentRad, const ColorRgbMutable *childRad);
    static void stchsRadElemPullImp( const StochasticRadiosityElement *parent, const StochasticRadiosityElement *child, double *parentImportance, const double *childImportance);

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

    RpkGeometry *getGeometry() const{ return geometry;
    }

    void setGeometry(RpkGeometry *inGeometry){ geometry = inGeometry;
    }

  private:
    static int coefficientPoolsInitialized;

    static void vertexAttachElement(RpkVertex *vertex, StochasticRadiosityElement *elem);
    static StochasticRadiosityElement *createElement();
    static StochasticRadiosityElement *mntCarloRadCreateClust(RpkGeometry *geometry);
    static void mntCarloRadCreateSurfElemChld(Patch *patch, StochasticRadiosityElement *parent);
    static void mntCarloRadCreateClustChld(RpkGeometry *geometry, StochasticRadiosityElement *parent);
    static void mntCarloRadInitClustPull(StochasticRadiosityElement *parent, const StochasticRadiosityElement *child);
    static void mntCarloRadCreateClustChildren(StochasticRadiosityElement *parent);
    static StochasticRadiosityElement *mntCarloRadCreateClustHierRec(RpkGeometry *world);
    static Vector3Dd galerkinElementMidpoint(StochasticRadiosityElement *elem);
    static Vector3Dd *mntCarloRadInstCoord(const Vector3Dd *coord);
    static Vector3Dd *mntCarloRadInstNorm(const Vector3Dd *normal);
    static Vector3Dd *mntCarloRadInstTexCoord(const Vector3Dd *texCoord);
    static RpkVertex *mntCarloRadInstVtx(Vector3Dd *coord, Vector3Dd *normal, Vector3Dd *texCoord);
    static RpkVertex *mntCarloRadNewMidVtx( StochasticRadiosityElement *elem, const RpkVertex *vertex1, const RpkVertex *vertex2);
    static StochasticRadiosityElement *mntCarloRadElemNghbr( const StochasticRadiosityElement *elem, int edgeNumber);
    static RpkVertex *mntCarloRadNewEdgeMidVtx(StochasticRadiosityElement *elem, int edgeNumber);
    static void mntCarloRadElemCompAvgReflAEmit(StochasticRadiosityElement *elem);
    static void mntCarloRadInitSurfPush(const StochasticRadiosityElement *parent, StochasticRadiosityElement *child);
    static StochasticRadiosityElement *mntCarloRadCreateSurfSubElem( StochasticRadiosityElement *parent, int childNumber, RpkVertex *v0, RpkVertex *v1, RpkVertex *v2, RpkVertex *v3);
    static StochasticRadiosityElement **mntCarloRadRegSbdvdTri( StochasticRadiosityElement *element, const RenderOptions *renderOptions);
    static StochasticRadiosityElement **mntCarloRadRegSbdvdQuad( StochasticRadiosityElement *element, const RenderOptions *renderOptions);
    static void mntCarloRadDestroyElem(StochasticRadiosityElement *elem);
    static void mntCarloRadDestroySurfElem(StochasticRadiosityElement *elem);
    static bool regularChild(const StochasticRadiosityElement *child);
    static ColorRgb vertexRadiance(const RpkVertex *vertex);
    static double vertexImportance(const RpkVertex *vertex);
    static ColorRgb vertexColor(RpkVertex *vertex);
};

#endif

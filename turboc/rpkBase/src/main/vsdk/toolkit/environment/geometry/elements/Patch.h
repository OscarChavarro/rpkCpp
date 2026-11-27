#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __PATCH__
#define __PATCH__

#include "vsdk/toolkit/common/linealAlgebra/Jacobian.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/material/RpkMaterial.h"
#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"

#include "vsdk/toolkit/environment/geometry/elements/PatchConstants.h"
#include "vsdk/toolkit/environment/geometry/elements/RayExclusionSet.h"

class RpkRayHit;

class Patch {
  private:
    #define TOLERANCE 1e-5

    static void
    pointInTriangle(
        const Vector3Dd &v0,
        const Vector3Dd &v1,
        const Vector3Dd &v2,
        double u,
        double v,
        Vector3Dd &p);

    static void
    pointInQuadrilateral(
        const Vector3Dd &v0,
        const Vector3Dd &v1,
        const Vector3Dd &v2,
        const Vector3Dd &v3,
        double u,
        double v,
        Vector3Dd &p);

    // A static counter which is increased every time a Patch is created in
    // order to make a unique Patch id
    static int patchId;
    unsigned char flags; // Other flags
    int faceIndex; // Dense zero-based index in the scene's face table, -1 until the scene assigns it

    static double clipToUnitInterval(double x);
    static bool solveQuadraticUnitInterval(double A, double B, double C, double *x);
    static bool quadUv(const Patch *patch, const Vector3Dd *point, Vector2Dd *uv);
    static Vector3Dd *patchNormal(const Patch *patch, Vector3Dd *normal);

    void uniformToBiLinear(double *u, double *v) const;
    Vector3Dd interpolatedNormalAtUv(double u, double v) const;
    int getNumberOfSamples() const;
    bool hitInPatch(RpkRayHit *hit, const Patch *patch) const;
    bool allVerticesHaveANormal() const;
    Vector3Dd getInterpolatedNormalAtUv(double u, double v) const;
    double computeTolerance() const;
    bool triangleUv(const Vector3Dd *point, Vector2Dd *uv) const;
    bool isAtLeastPartlyInFront(const Patch *other) const;
    void connectVertex(RpkVertex *paramVertex);
    void connectVertices();
    double computeRandomWalkRadiosityArea();
    void computeMidpoint(Vector3Dd *p) const;

  public:
    unsigned id; // Identification number for debugging, ID rendering
    Patch *twin; // Twin face (for double-sided surfaces)
    RpkVertex *vertex[MAXIMUM_VERTICES_PER_PATCH]; // Pointers to the vertices
    char numberOfVertices; // Number of vertices: 3 or 4
    RpkAxisAlignedBoundingBox *boundingBox;
    Vector3Dd normal;
    double planeConstant;
    double tolerance; // Plane tolerance
    double area; // Patch area
    Vector3Dd midPoint; // Patch midpoint
    Jacobian *jacobian; // Shape-related constants for irregular quadrilaterals.
                        // Used for sampling the quadrilateral and for computing integrals
    double directPotential; // Directly received hemispherical potential (ref: Pattanaik, ACM Trans Graph, 1995?).
                           // Only determined when asked to do so (see potential.[ch]).
    char index; // Indicates dominant part of patch normal
    char omit; // Indicates that the patch should not be considered
               // for a couple of things, such as intersection
               // testing, shaft culling, ... set to FALSE by
               // default. Don't forget to set to FALSE again
			   // after you changed it!
    ColorRgb color; // Color used to flat render the patch
    Element *radianceData; // Data needed for radiance computations. Content depends on the current radiance algorithm / radiosity method (a.k.a. context)
    RpkMaterial *material;

    static int getNextId();
    static void setNextId(int id);

    Patch(int inNumberOfVertices, RpkVertex *v1, RpkVertex *v2, RpkVertex *v3, RpkVertex *v4);
    ~Patch();

    void setVisible();
    void setInvisible();
    void setFlags(unsigned char newFlags);
    unsigned char getFlags() const;
    int getFaceIndex() const;
    void setFaceIndex(int newFaceIndex);
    double getTolerance() const;
    bool hasZeroVertices() const;
    Vector3Dd *pointBarycentricMapping(double u, double v, Vector3Dd *point) const;
    Vector3Dd *uniformPoint(double u, double v, Vector3Dd *point) const;
    bool uv(const Vector3Dd *point, double *u, double *v) const;
    void biLinearToUniform(double *u, double *v) const;
    void interpolatedFrameAtUv(double u, double v, Vector3Dd *X, Vector3Dd *Y, Vector3Dd *Z) const;
    Vector3Dd textureCoordAtUv(double u, double v) const;
    void computeVertexColors() const;
    bool facing(const Patch *other) const;
    void computeBoundingBox();
    void computeAndGetBoundingBox(RpkAxisAlignedBoundingBox *bounds);
    RpkRayHit *intersect(const Ray *ray, double minimumDistance, double *maximumDistance, int hitFlags, RpkRayHit *hitStore, const RayExclusionSet &exclusions);

#ifdef RAYTRACING_ENABLED
    bool isVisible() const;
    bool uniformUv(const Vector3Dd *point, double *u, double *v) const;

#endif

};

inline double
Patch::clipToUnitInterval(double x) {
    if ( x < VSDK::EPSILON ) {
        return VSDK::EPSILON;
    } else {
        return x > (1.0 - VSDK::EPSILON) ? 1.0 - VSDK::EPSILON : x;
    }
}

inline void
Patch::setVisible() {
    flags |= PATCH_VISIBILITY;
}

inline void
Patch::setInvisible() {
    flags &= ~PATCH_VISIBILITY;
}

inline void
Patch::setFlags(unsigned char newFlags) {
    flags = newFlags;
}

inline unsigned char
Patch::getFlags() const {
    return flags;
}

inline int
Patch::getFaceIndex() const {
    return faceIndex;
}

inline void
Patch::setFaceIndex(const int newFaceIndex) {
    faceIndex = newFaceIndex;
}

inline double
Patch::getTolerance() const {
    return tolerance;
}

#ifdef RAYTRACING_ENABLED
inline bool
Patch::isVisible() const {
    return (flags & PATCH_VISIBILITY) != 0;
}

#endif

#endif

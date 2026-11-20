#ifndef PATCH__
#define PATCH__

#include "vsdk/toolkit/common/linealAlgebra/Jacobian.h"
#include "vsdk/toolkit/common/linealAlgebra/RpkRay.h"
#include "vsdk/toolkit/material/RpkMaterial.h"
#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"
#include "vsdk/toolkit/environment/geometry/elements/PatchConstants.h"
#include "vsdk/toolkit/environment/geometry/elements/RayExclusionSet.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"

class RpkRayHit;

class FaceColorTable;

class Patch {
  private:
    static constexpr double TOLERANCE = 1e-5;

    // A static counter which is increased every time a Patch is created in
    // order to make a unique Patch id
    static int patchId;

    static void
    pointInTriangle(
        const Vector3D &v0,
        const Vector3D &v1,
        const Vector3D &v2,
        float u,
        float v,
        Vector3D &p);

    static void
    pointInQuadrilateral(
        const Vector3D &v0,
        const Vector3D &v1,
        const Vector3D &v2,
        const Vector3D &v3,
        float u,
        float v,
        Vector3D &p);

    static double clipToUnitInterval(double x);
    static bool solveQuadraticUnitInterval(double A, double B, double C, double *x);
    static bool quadUv(const Patch *patch, const Vector3D *point, Vector2Dd *uv);
    static Vector3D *patchNormal(const Patch *patch, Vector3D *normal);

    Vector3D patchMidPoint; // Patch midpoint
    RpkMaterial *material;
    unsigned id; // Identification number for debugging, ID rendering
    int faceIndex; // Dense zero-based index in the scene's face table, -1 until the scene assigns it
    Patch *twin; // Twin face (for double-sided surfaces)
    const char numberOfVertices; // Number of vertices: 3 or 4
    RpkVertex *const vertex[MAXIMUM_VERTICES_PER_PATCH]; // Pointers to the vertices
    RpkAxisAlignedBoundingBox *const boundingBox;
    float planeTolerance; // Plane tolerance
    // Only determined when asked to do so (see potential.[ch]).
    Vector3D normal;
    float planeConstant;
    float area; // Patch area
    Jacobian *jacobian; // Shape-related constants for irregular quadrilaterals.
                        // Used for sampling the quadrilateral and for computing integrals
    Element *radianceData; // Data needed for radiance computations. Content depends on the current radiance algorithm / radiosity method (a.k.a. context)
    char index; // Indicates dominant part of patch normal
    char omit; // Indicates that the patch should not be considered
    // for a couple of things, such as intersection
    // testing, shaft culling, ... set to FALSE by
    // default. Don't forget to set to FALSE again
    // after you changed it!

    void uniformToBiLinear(double *u, double *v) const;
    Vector3D interpolatedNormalAtUv(double u, double v) const;
    bool hitInPatch(RpkRayHit *hit, const Patch *patch) const;
    bool allVerticesHaveANormal() const;
    Vector3D getInterpolatedNormalAtUv(double u, double v) const;
    float computeTolerance() const;
    bool triangleUv(const Vector3D *point, Vector2Dd *uv) const;
    bool isAtLeastPartlyInFront(const Patch *other) const;
    void connectVertex(RpkVertex *paramVertex);
    void connectVertices();
    float computeRandomWalkRadiosityArea();
    void computeMidpoint(Vector3D *p) const;

  public:
    static int getNextId();
    static void setNextId(int id);

    Patch(int inNumberOfVertices, RpkVertex *v1, RpkVertex *v2, RpkVertex *v3, RpkVertex *v4);
    ~Patch();

    void biLinearToUniform(double *u, double *v) const;
    void computeAndGetBoundingBox(RpkAxisAlignedBoundingBox *bounds);
    void computeBoundingBox();
    void computeVertexColors(const FaceColorTable *faceColors) const;
    bool facing(const Patch *other) const;
    float getArea() const;
    const RpkAxisAlignedBoundingBox *getBoundingBox() const;
    char getDominantAxisIndex() const;
    unsigned getId() const;
    int getFaceIndex() const;
    const Jacobian *getJacobian() const;
    const RpkMaterial *getMaterial() const;
    const Vector3D &getNormal() const;
    char getNumberOfVertices() const;
    float getPlaneConstant() const;
    Element *getRadianceData() const;
    float getTolerance() const;
    Patch *const &getTwin() const;
    RpkVertex *const (&getVertices() const)[MAXIMUM_VERTICES_PER_PATCH];
    bool hasZeroVertices() const;
    void interpolatedFrameAtUv(double u, double v, Vector3D *X, Vector3D *Y, Vector3D *Z) const;
    RpkRayHit *intersect(const RpkRay *ray, float minimumDistance, float *maximumDistance, int hitFlags, RpkRayHit *hitStore, const RayExclusionSet &exclusions) const;
    bool isOmitted() const;
    const Vector3D &midPoint() const;
    Vector3D *pointBarycentricMapping(double u, double v, Vector3D *point) const;
    void setDominantAxisIndex(char newIndex);
    void setId(unsigned newId);
    void setFaceIndex(int newFaceIndex);
    void setJacobian(Jacobian *newJacobian);
    void setMaterial(RpkMaterial *newMaterial);
    void setOmit(bool shouldOmit);
    void setRadianceData(Element *newRadianceData);
    void setTwin(Patch *newTwin);
    Vector3D textureCoordAtUv(double u, double v) const;
    Vector3D *uniformPoint(double u, double v, Vector3D *point) const;
    bool uv(const Vector3D *point, double *u, double *v) const;

#ifdef RAYTRACING_ENABLED
    bool uniformUv(const Vector3D *point, double *u, double *v) const;
#endif
};

inline double
Patch::clipToUnitInterval(double x) {
    if ( x < Numeric::EPSILON ) {
        return Numeric::EPSILON;
    } else {
        return x > (1.0 - Numeric::EPSILON) ? 1.0 - Numeric::EPSILON : x;
    }
}

inline float
Patch::getArea() const {
    return area;
}

inline const RpkAxisAlignedBoundingBox *
Patch::getBoundingBox() const {
    return boundingBox;
}

inline char
Patch::getDominantAxisIndex() const {
    return index;
}

inline int
Patch::getFaceIndex() const {
    return faceIndex;
}

inline void
Patch::setFaceIndex(const int newFaceIndex) {
    faceIndex = newFaceIndex;
}

inline unsigned
Patch::getId() const {
    return id;
}

inline const Jacobian *
Patch::getJacobian() const {
    return jacobian;
}

inline const RpkMaterial *
Patch::getMaterial() const {
    return material;
}

inline const Vector3D &
Patch::getNormal() const {
    return normal;
}

inline char
Patch::getNumberOfVertices() const {
    return numberOfVertices;
}

inline float
Patch::getPlaneConstant() const {
    return planeConstant;
}

inline Element *
Patch::getRadianceData() const {
    return radianceData;
}

inline float
Patch::getTolerance() const {
    return planeTolerance;
}

inline Patch *const &
Patch::getTwin() const {
    return twin;
}

inline RpkVertex *const (&
Patch::getVertices() const)[MAXIMUM_VERTICES_PER_PATCH] {
    return vertex;
}

inline bool
Patch::isOmitted() const {
    return omit != 0;
}

inline const Vector3D &
Patch::midPoint() const {
    return patchMidPoint;
}

inline void
Patch::setDominantAxisIndex(char newIndex) {
    index = newIndex;
}

inline void
Patch::setId(unsigned newId) {
    id = newId;
}

inline void
Patch::setJacobian(Jacobian *newJacobian) {
    if ( jacobian != nullptr ) {
        delete jacobian;
    }
    jacobian = newJacobian;
}

inline void
Patch::setMaterial(RpkMaterial *newMaterial) {
    material = newMaterial;
}

inline void
Patch::setOmit(bool shouldOmit) {
    omit = static_cast<char>(shouldOmit ? 1 : 0);
}

inline void
Patch::setRadianceData(Element *newRadianceData) {
    radianceData = newRadianceData;
}

inline void
Patch::setTwin(Patch *newTwin) {
    twin = newTwin;
}

#endif

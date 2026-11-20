#ifndef PATCH__
#define PATCH__

#include "vsdk/toolkit/common/linealAlgebra/Jacobian.h"
#include "vsdk/toolkit/common/linealAlgebra/RpkRay.h"
#include "vsdk/toolkit/environment/geometry/element/PlanarFace.h"
#include "vsdk/toolkit/material/RpkMaterial.h"
#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"
#include "vsdk/toolkit/environment/geometry/elements/PatchConstants.h"
#include "vsdk/toolkit/environment/geometry/elements/RayExclusionSet.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"

class RpkRayHit;

class FaceColorTable;

class Patch {
  private:
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

    static Vector3Df toFace(const Vector3D &v);
    static Vector3D fromFace(const Vector3Df &v);

    // Geometry of the face: positions, plane, area, parametrization; immutable
    PlanarFace face;
    Vector3D normal; // face.getNormal(), in the type RPK's callers use
    Vector3D patchMidPoint; // face.getMidPoint(), in the type RPK's callers use
    RpkMaterial *material;
    unsigned id; // Identification number for debugging, ID rendering
    int faceIndex; // Dense zero-based index in the scene's face table, -1 until the scene assigns it
    Patch *twin; // Twin face (for double-sided surfaces)
    const char numberOfVertices; // Number of vertices: 3 or 4
    RpkVertex *const vertex[MAXIMUM_VERTICES_PER_PATCH]; // Pointers to the vertices
    Jacobian *jacobian; // face's Jacobian coefficients, for irregular quadrilaterals only

    Vector3D interpolatedNormalAtUv(double u, double v) const;
    bool allVerticesHaveANormal() const;
    Vector3D getInterpolatedNormalAtUv(double u, double v) const;
    void connectVertex(RpkVertex *paramVertex);
    void connectVertices();

  public:
    static int getNextId();
    static void setNextId(int id);

    Patch(int inNumberOfVertices, RpkVertex *v1, RpkVertex *v2, RpkVertex *v3, RpkVertex *v4);
    ~Patch();

    void biLinearToUniform(double *u, double *v) const;
    void computeBounds(RpkAxisAlignedBoundingBox *bounds) const;
    void computeVertexColors(const FaceColorTable *faceColors) const;
    bool facing(const Patch *other) const;
    float getArea() const;
    const PlanarFace &getFace() const;
    char getDominantAxisIndex() const;
    unsigned getId() const;
    int getFaceIndex() const;
    const Jacobian *getJacobian() const;
    const RpkMaterial *getMaterial() const;
    const Vector3D &getNormal() const;
    char getNumberOfVertices() const;
    float getPlaneConstant() const;
    float getTolerance() const;
    Patch *const &getTwin() const;
    RpkVertex *const (&getVertices() const)[MAXIMUM_VERTICES_PER_PATCH];
    bool hasZeroVertices() const;
    void interpolatedFrameAtUv(double u, double v, Vector3D *X, Vector3D *Y, Vector3D *Z) const;
    RpkRayHit *intersect(const RpkRay *ray, float minimumDistance, float *maximumDistance, int hitFlags, RpkRayHit *hitStore, const RayExclusionSet &exclusions) const;
    const Vector3D &midPoint() const;
    Vector3D *pointBarycentricMapping(double u, double v, Vector3D *point) const;
    void setId(unsigned newId);
    void setFaceIndex(int newFaceIndex);
    void setMaterial(RpkMaterial *newMaterial);
    void setTwin(Patch *newTwin);
    Vector3D textureCoordAtUv(double u, double v) const;
    Vector3D *uniformPoint(double u, double v, Vector3D *point) const;
    bool uv(const Vector3D *point, double *u, double *v) const;

#ifdef RAYTRACING_ENABLED
    bool uniformUv(const Vector3D *point, double *u, double *v) const;
#endif
};

inline Vector3Df
Patch::toFace(const Vector3D &v) {
    return Vector3Df(v.x, v.y, v.z);
}

inline Vector3D
Patch::fromFace(const Vector3Df &v) {
    return Vector3D(v.x(), v.y(), v.z());
}

inline float
Patch::getArea() const {
    return face.getArea();
}

inline const PlanarFace &
Patch::getFace() const {
    return face;
}

inline char
Patch::getDominantAxisIndex() const {
    return static_cast<char>(face.getDominantAxis());
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
    return face.getPlaneConstant();
}

inline float
Patch::getTolerance() const {
    return face.getTolerance();
}

inline Patch *const &
Patch::getTwin() const {
    return twin;
}

inline RpkVertex *const (&
Patch::getVertices() const)[MAXIMUM_VERTICES_PER_PATCH] {
    return vertex;
}

inline const Vector3D &
Patch::midPoint() const {
    return patchMidPoint;
}

inline void
Patch::setId(unsigned newId) {
    id = newId;
}

inline void
Patch::setMaterial(RpkMaterial *newMaterial) {
    material = newMaterial;
}

inline void
Patch::setTwin(Patch *newTwin) {
    twin = newTwin;
}

#endif

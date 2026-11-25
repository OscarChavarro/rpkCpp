#ifndef __PATCH__
#define __PATCH__

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
        const Vector3Df &v0,
        const Vector3Df &v1,
        const Vector3Df &v2,
        float u,
        float v,
        Vector3Df &p);

    static void
    pointInQuadrilateral(
        const Vector3Df &v0,
        const Vector3Df &v1,
        const Vector3Df &v2,
        const Vector3Df &v3,
        float u,
        float v,
        Vector3Df &p);

    // Geometry of the face: positions, plane, area, parametrization; immutable
    PlanarFace face;
    RpkMaterial *material;
    unsigned id; // Identification number for debugging, ID rendering
    int faceIndex; // Dense zero-based index in the scene's face table, -1 until the scene assigns it
    Patch *twin; // Twin face (for double-sided surfaces)
    const char numberOfVertices; // Number of vertices: 3 or 4
    RpkVertex *const vertex[MAXIMUM_VERTICES_PER_PATCH]; // Pointers to the vertices

    Vector3Df interpolatedNormalAtUv(double u, double v) const;
    bool allVerticesHaveANormal() const;
    Vector3Df getInterpolatedNormalAtUv(double u, double v) const;
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
    const RpkMaterial *getMaterial() const;
    Vector3Df getNormal() const;
    char getNumberOfVertices() const;
    float getPlaneConstant() const;
    float getTolerance() const;
    Patch *const &getTwin() const;
    RpkVertex *const (&getVertices() const)[MAXIMUM_VERTICES_PER_PATCH];
    bool hasZeroVertices() const;
    void interpolatedFrameAtUv(double u, double v, Vector3Df *X, Vector3Df *Y, Vector3Df *Z) const;
    RpkRayHit *intersect(const RpkRay *ray, float minimumDistance, float *maximumDistance, int hitFlags, RpkRayHit *hitStore, const RayExclusionSet &exclusions) const;
    Vector3Df midPoint() const;
    Vector3Df *pointBarycentricMapping(double u, double v, Vector3Df *point) const;
    void setId(unsigned newId);
    void setFaceIndex(int newFaceIndex);
    void setMaterial(RpkMaterial *newMaterial);
    void setTwin(Patch *newTwin);
    Vector3Df textureCoordAtUv(double u, double v) const;
    Vector3Df *uniformPoint(double u, double v, Vector3Df *point) const;
    bool uv(const Vector3Df *point, double *u, double *v) const;

#ifdef RAYTRACING_ENABLED
    bool uniformUv(const Vector3Df *point, double *u, double *v) const;
#endif
};

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

inline const RpkMaterial *
Patch::getMaterial() const {
    return material;
}

inline Vector3Df
Patch::getNormal() const {
    return face.getNormal();
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

inline Vector3Df
Patch::midPoint() const {
    return face.getMidPoint();
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

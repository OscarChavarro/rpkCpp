#ifndef __PATCH__
#define __PATCH__

#include "common/linealAlgebra/Jacobian.h"
#include "common/linealAlgebra/Ray.h"
#include "material/Material.h"
#include "skin/BoundingBox.h"
#include "skin/PatchConstants.h"
#include "skin/Vertex.h"

class Patch {
  private:
    static constexpr double TOLERANCE = 1e-5;

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

    // A static counter which is increased every time a Patch is created in
    // order to make a unique Patch id
    static int patchId;
    static Patch *excludedPatches[MAX_EXCLUDED_PATCHES];
    static void
    dontIntersectBase(
        int n,
        Patch *p0,
        Patch *p1,
        Patch *p2,
        Patch *p3);

    unsigned char flags; // Other flags
    Vector3D patchMidPoint; // Patch midpoint
    Material *material;

    static double clipToUnitInterval(double x);
    static bool solveQuadraticUnitInterval(double A, double B, double C, double *x);
    static bool quadUv(const Patch *patch, const Vector3D *point, Vector2Dd *uv);
    static Vector3D *patchNormal(const Patch *patch, Vector3D *normal);

    void uniformToBiLinear(double *u, double *v) const;
    Vector3D interpolatedNormalAtUv(double u, double v) const;
    bool isExcluded() const;
    bool hitInPatch(RayHit *hit, const Patch *patch) const;
    bool allVerticesHaveANormal() const;
    Vector3D getInterpolatedNormalAtUv(double u, double v) const;
    float computeTolerance() const;
    bool triangleUv(const Vector3D *point, Vector2Dd *uv) const;
    bool isAtLeastPartlyInFront(const Patch *other) const;
    void connectVertex(Vertex *paramVertex);
    void connectVertices();
    float computeRandomWalkRadiosityArea();
    void computeMidpoint(Vector3D *p) const;

  public:
    unsigned id; // Identification number for debugging, ID rendering
    Patch *twin; // Twin face (for double-sided surfaces)
    Vertex *vertex[MAXIMUM_VERTICES_PER_PATCH]; // Pointers to the vertices
    char numberOfVertices; // Number of vertices: 3 or 4
    BoundingBox *boundingBox;
    Vector3D normal;
    float planeConstant;
    float tolerance; // Plane tolerance
    float area; // Patch area
    Jacobian *jacobian; // Shape-related constants for irregular quadrilaterals.
                        // Used for sampling the quadrilateral and for computing integrals
    float directPotential; // Directly received hemispherical potential (ref: Pattanaik, ACM Trans Graph, 1995?).
                           // Only determined when asked to do so (see potential.[ch]).
    char index; // Indicates dominant part of patch normal
    char omit; // Indicates that the patch should not be considered
               // for a couple of things, such as intersection
               // testing, shaft culling, ... set to FALSE by
               // default. Don't forget to set to FALSE again
               // after you changed it!
    ColorRgb color; // Color used to flat render the patch
    Element *radianceData; // Data needed for radiance computations. Content depends on the current radiance algorithm / radiosity method (a.k.a. context)

    static void dontIntersect0();
    static void dontIntersect2(Patch *p0, Patch *p1);
    static void dontIntersect3(Patch *p0, Patch *p1, Patch *p2);
    static void dontIntersect4(Patch *p0, Patch *p1, Patch *p2, Patch *p3);
    static int getNextId();
    static void setNextId(int id);

    Patch(int inNumberOfVertices, Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4);
    ~Patch();

    void setVisible();
    void setInvisible();
    void setFlags(unsigned char newFlags);
    unsigned char getFlags() const;
    unsigned getId() const;
    void setId(unsigned newId);
    Patch *getTwin() const;
    void setTwin(Patch *newTwin);
    const Vector3D &getNormal() const;
    void setNormal(const Vector3D &newNormal);
    float getPlaneConstant() const;
    void setPlaneConstant(float newPlaneConstant);
    float getTolerance() const;
    void setTolerance(float newTolerance);
    float getArea() const;
    void setArea(float newArea);
    float getDirectPotential() const;
    void setDirectPotential(float newDirectPotential);
    char getDominantAxisIndex() const;
    void setDominantAxisIndex(char newIndex);
    bool isOmitted() const;
    void setOmit(bool shouldOmit);
    const ColorRgb &getColor() const;
    void setColor(const ColorRgb &newColor);
    const Material *getMaterial() const;
    void setMaterial(Material *newMaterial);
    bool hasZeroVertices() const;
    Vector3D *pointBarycentricMapping(double u, double v, Vector3D *point) const;
    Vector3D *uniformPoint(double u, double v, Vector3D *point) const;
    bool uv(const Vector3D *point, double *u, double *v) const;
    void biLinearToUniform(double *u, double *v) const;
    void interpolatedFrameAtUv(double u, double v, Vector3D *X, Vector3D *Y, Vector3D *Z) const;
    Vector3D textureCoordAtUv(double u, double v) const;
    void computeVertexColors() const;
    bool facing(const Patch *other) const;
    const Vector3D &midPoint() const;
    void computeBoundingBox();
    void computeAndGetBoundingBox(BoundingBox *bounds);
    RayHit *intersect(const Ray *ray, float minimumDistance, float *maximumDistance, int hitFlags, RayHit *hitStore);

#ifdef RAYTRACING_ENABLED
    bool isVisible() const;
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

inline unsigned
Patch::getId() const {
    return id;
}

inline void
Patch::setId(unsigned newId) {
    id = newId;
}

inline Patch *
Patch::getTwin() const {
    return twin;
}

inline void
Patch::setTwin(Patch *newTwin) {
    twin = newTwin;
}

inline const Vector3D &
Patch::getNormal() const {
    return normal;
}

inline void
Patch::setNormal(const Vector3D &newNormal) {
    normal = newNormal;
}

inline float
Patch::getPlaneConstant() const {
    return planeConstant;
}

inline void
Patch::setPlaneConstant(float newPlaneConstant) {
    planeConstant = newPlaneConstant;
}

inline float
Patch::getTolerance() const {
    return tolerance;
}

inline void
Patch::setTolerance(float newTolerance) {
    tolerance = newTolerance;
}

inline float
Patch::getArea() const {
    return area;
}

inline void
Patch::setArea(float newArea) {
    area = newArea;
}

inline float
Patch::getDirectPotential() const {
    return directPotential;
}

inline void
Patch::setDirectPotential(float newDirectPotential) {
    directPotential = newDirectPotential;
}

inline char
Patch::getDominantAxisIndex() const {
    return index;
}

inline void
Patch::setDominantAxisIndex(char newIndex) {
    index = newIndex;
}

inline bool
Patch::isOmitted() const {
    return omit != 0;
}

inline void
Patch::setOmit(bool shouldOmit) {
    omit = static_cast<char>(shouldOmit ? 1 : 0);
}

inline const ColorRgb &
Patch::getColor() const {
    return color;
}

inline void
Patch::setColor(const ColorRgb &newColor) {
    color = newColor;
}

inline const Material *
Patch::getMaterial() const {
    return material;
}

inline void
Patch::setMaterial(Material *newMaterial) {
    material = newMaterial;
}

inline const Vector3D &
Patch::midPoint() const {
    return patchMidPoint;
}

#ifdef RAYTRACING_ENABLED
inline bool
Patch::isVisible() const {
    return (flags & PATCH_VISIBILITY) != 0;
}
#endif

#endif

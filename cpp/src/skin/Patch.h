#ifndef __PATCH__
#define __PATCH__

#include "common/linealAlgebra/Jacobian.h"
#include "common/linealAlgebra/Ray.h"
#include "material/Material.h"
#include "skin/BoundingBox.h"
#include "skin/PatchConstants.h"
#include "skin/Vertex.h"

class RayHit;

class Patch {
  private:
    static constexpr double TOLERANCE = 1e-5;

    // A static counter which is increased every time a Patch is created in
    // order to make a unique Patch id
    static int patchId;
    static Patch *excludedPatches[MAX_EXCLUDED_PATCHES];

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

    static void
    dontIntersectBase(
        int n,
        Patch *p0,
        Patch *p1,
        Patch *p2,
        Patch *p3);

    static double clipToUnitInterval(double x);
    static bool solveQuadraticUnitInterval(double A, double B, double C, double *x);
    static bool quadUv(const Patch *patch, const Vector3D *point, Vector2Dd *uv);
    static Vector3D *patchNormal(const Patch *patch, Vector3D *normal);

    unsigned char flags; // Other flags
    Vector3D patchMidPoint; // Patch midpoint
    Material *material;
    unsigned id; // Identification number for debugging, ID rendering
    Patch *twin; // Twin face (for double-sided surfaces)
    const char numberOfVertices; // Number of vertices: 3 or 4
    Vertex *const vertex[MAXIMUM_VERTICES_PER_PATCH]; // Pointers to the vertices
    BoundingBox *const boundingBox;
    float planeTolerance; // Plane tolerance
    float directPotential; // Directly received hemispherical potential (ref: Pattanaik, ACM Trans Graph, 1995?).
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
    ColorRgb color; // Color used to flat render the patch

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
    static void dontIntersect0();
    static void dontIntersect2(Patch *p0, Patch *p1);
    static void dontIntersect3(Patch *p0, Patch *p1, Patch *p2);
    static void dontIntersect4(Patch *p0, Patch *p1, Patch *p2, Patch *p3);
    static int getNextId();
    static void setNextId(int id);

    Patch(int inNumberOfVertices, Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4);
    ~Patch();

    void biLinearToUniform(double *u, double *v) const;
    void computeAndGetBoundingBox(BoundingBox *bounds);
    void computeBoundingBox();
    void computeVertexColors() const;
    bool facing(const Patch *other) const;
    float getArea() const;
    const BoundingBox *getBoundingBox() const;
    const ColorRgb &getColor() const;
    float getDirectPotential() const;
    char getDominantAxisIndex() const;
    unsigned char getFlags() const;
    unsigned getId() const;
    const Jacobian *getJacobian() const;
    const Material *getMaterial() const;
    const Vector3D &getNormal() const;
    char getNumberOfVertices() const;
    float getPlaneConstant() const;
    Element *getRadianceData() const;
    float getTolerance() const;
    Patch *const &getTwin() const;
    Vertex *const (&getVertices() const)[MAXIMUM_VERTICES_PER_PATCH];
    bool hasZeroVertices() const;
    void interpolatedFrameAtUv(double u, double v, Vector3D *X, Vector3D *Y, Vector3D *Z) const;
    RayHit *intersect(const Ray *ray, float minimumDistance, float *maximumDistance, int hitFlags, RayHit *hitStore);
    bool isOmitted() const;
    const Vector3D &midPoint() const;
    Vector3D *pointBarycentricMapping(double u, double v, Vector3D *point) const;
    void setColor(const ColorRgb &newColor);
    void setDirectPotential(float newDirectPotential);
    void setDominantAxisIndex(char newIndex);
    void setFlags(unsigned char newFlags);
    void setId(unsigned newId);
    void setInvisible();
    void setJacobian(Jacobian *newJacobian);
    void setMaterial(Material *newMaterial);
    void setOmit(bool shouldOmit);
    void setRadianceData(Element *newRadianceData);
    void setTwin(Patch *newTwin);
    void setVisible();
    Vector3D textureCoordAtUv(double u, double v) const;
    Vector3D *uniformPoint(double u, double v, Vector3D *point) const;
    bool uv(const Vector3D *point, double *u, double *v) const;

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

inline float
Patch::getArea() const {
    return area;
}

inline const BoundingBox *
Patch::getBoundingBox() const {
    return boundingBox;
}

inline const ColorRgb &
Patch::getColor() const {
    return color;
}

inline float
Patch::getDirectPotential() const {
    return directPotential;
}

inline char
Patch::getDominantAxisIndex() const {
    return index;
}

inline unsigned char
Patch::getFlags() const {
    return flags;
}

inline unsigned
Patch::getId() const {
    return id;
}

inline const Jacobian *
Patch::getJacobian() const {
    return jacobian;
}

inline const Material *
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

inline Vertex *const (&
Patch::getVertices() const)[MAXIMUM_VERTICES_PER_PATCH] {
    return vertex;
}

inline bool
Patch::isOmitted() const {
    return omit != 0;
}

#ifdef RAYTRACING_ENABLED
inline bool
Patch::isVisible() const {
    return (flags & PATCH_VISIBILITY) != 0;
}
#endif

inline const Vector3D &
Patch::midPoint() const {
    return patchMidPoint;
}

inline void
Patch::setColor(const ColorRgb &newColor) {
    color = newColor;
}

inline void
Patch::setDirectPotential(float newDirectPotential) {
    directPotential = newDirectPotential;
}

inline void
Patch::setDominantAxisIndex(char newIndex) {
    index = newIndex;
}

inline void
Patch::setFlags(unsigned char newFlags) {
    flags = newFlags;
}

inline void
Patch::setId(unsigned newId) {
    id = newId;
}

inline void
Patch::setInvisible() {
    flags &= ~PATCH_VISIBILITY;
}

inline void
Patch::setJacobian(Jacobian *newJacobian) {
    if ( jacobian != nullptr ) {
        delete jacobian;
    }
    jacobian = newJacobian;
}

inline void
Patch::setMaterial(Material *newMaterial) {
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

inline void
Patch::setVisible() {
    flags |= PATCH_VISIBILITY;
}

#endif

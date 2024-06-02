#ifndef __PATCH__
#define __PATCH__

#include "common/RenderOptions.h"
#include "material/Material.h"
#include "skin/BoundingBox.h"
#include "common/linealAlgebra/Jacobian.h"
#include "skin/Vertex.h"

#define MAXIMUM_VERTICES_PER_PATCH 4
#define PATCH_VISIBILITY 0x01
#define MAX_EXCLUDED_PATCHES 4

class Patch {
  private:
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
    static int globalPatchId;
    static Patch *globalExcludedPatches[MAX_EXCLUDED_PATCHES];

    unsigned char flags; // Other flags

    static double
    clipToUnitInterval(double x) {
        if ( x < Numeric::EPSILON ) {
            return Numeric::EPSILON;
        } else {
            return x > (1.0 - Numeric::EPSILON) ? 1.0 - Numeric::EPSILON : x;
        }
    }

    static int solveQuadraticUnitInterval(double A, double B, double C, double *x);
    static int quadUv(const Patch *patch, const Vector3D *point, Vector2Dd *uv);

    void uniformToBiLinear(double *u, double *v) const;
    Vector3D interpolatedNormalAtUv(double u, double v) const;
    int getNumberOfSamples() const;
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
    Vector3D midPoint; // Patch midpoint
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
    Material *material;

    static void dontIntersect(int n, ...);
    static int getNextId();
    static void setNextId(int id);

    Patch(int inNumberOfVertices, Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4);
    ~Patch();

    void
    setVisible() {
        flags |= PATCH_VISIBILITY;
    }

    void
    setInvisible() {
        flags &= ~PATCH_VISIBILITY;
    }

    int hasZeroVertices() const;
    Vector3D *pointBarycentricMapping(double u, double v, Vector3D *point) const;
    Vector3D *uniformPoint(double u, double v, Vector3D *point) const;
    int uv(const Vector3D *point, double *u, double *v) const;
    void biLinearToUniform(double *u, double *v) const;
    void interpolatedFrameAtUv(double u, double v, Vector3D *X, Vector3D *Y, Vector3D *Z) const;
    Vector3D textureCoordAtUv(double u, double v) const;
    void computeVertexColors() const;
    bool facing(const Patch *other) const;
    void computeBoundingBox();
    void computeAndGetBoundingBox(BoundingBox *bounds);
    RayHit *intersect(const Ray *ray, float minimumDistance, float *maximumDistance, int hitFlags, RayHit *hitStore);
    ColorRgb averageNormalAlbedo(char components);
    ColorRgb averageEmittance(char components);

#ifdef RAYTRACING_ENABLED
    bool
    isVisible() const {
        return (flags & PATCH_VISIBILITY) != 0;
    }

    int uniformUv(const Vector3D *point, double *u, double *v) const;

#endif

};

#endif

#ifndef __PATCH__
#define __PATCH__

#include "common/linealAlgebra/Vector3D.h"
#include "common/Ray.h"
#include "common/rgb.h"
#include "skin/Jacobian.h"
#include "skin/Vertex.h"
#include "skin/Element.h"
#include "skin/MeshSurface.h"
#include "material/hit.h"
#include "material/xxdf.h"

#define MAXIMUM_VERTICES_PER_PATCH 4
#define PATCH_VISIBILITY 0x01
#define MAX_EXCLUDED_PATCHES 4

class Patch {
  private:
    // A static counter which is increased every time a Patch is created in
    //order to make a unique Patch id
    static int globalPatchId;
    static Patch *globalExcludedPatches[MAX_EXCLUDED_PATCHES];

    unsigned char flags; // Other flags

    static double
    clipToUnitInterval(double x) {
        return x < EPSILON ? EPSILON : (x > (1.0 - EPSILON) ? 1.0 - EPSILON : x);
    }

    static int solveQuadraticUnitInterval(double A, double B, double C, double *x);
    static int quadUv(Patch *patch, Vector3D *point, Vector2Dd *uv);

    void uniformToBiLinear(double *u, double *v) const;
    Vector3D interpolatedNormalAtUv(double u, double v);
    int getNumberOfSamples();
    bool isExcluded();
    bool hitInPatch(RayHit *hit, Patch *patch);
    bool allVerticesHaveANormal();
    Vector3D getInterpolatedNormalAtUv(double u, double v);
    void patchConnectVertex(Vertex *paramVertex);
    void patchConnectVertices();
    float randomWalkRadiosityPatchArea();
    Vector3D *computeMidpoint(Vector3D *p);
    float computeTolerance();
    bool triangleUv(Vector3D *point, Vector2Dd *uv);

public:
    unsigned id; // Identification number for debugging, ID rendering
    Patch *twin; // Twin face (for double-sided surfaces)
    Vertex *vertex[MAXIMUM_VERTICES_PER_PATCH]; // Pointers to the vertices
    char numberOfVertices; // Number of vertices: 3 or 4
    float *boundingBox;
    Vector3D normal;
    float planeConstant;
    float tolerance; // Plane tolerance
    float area; // Patch area
    Vector3D midpoint; // Patch midpoint
    Jacobian *jacobian; // Shape-related constants for irregular quadrilaterals.
                        // Used for sampling the quadrilateral and for computing integrals
    float directPotential; // Directly received hemispherical potential (ref: Pattanaik, ACM Trans Graph).
                           // Only determined when asked to do so (see potential.[ch]).
    char index; // Indicates dominant part of patch normal
    char omit; // Indicates that the patch should not be considered
               // for a couple of things, such as intersection
               // testing, shaft culling, ... set to FALSE by
               // default. Don't forget to set to FALSE again
			   // after you changed it!
    RGB color; // Color used to flat render the patch
    Element *radianceData; // Data needed for radiance computations. Type depends on the current radiance algorithm
    MeshSurface *surface; // Pointer to surface data (contains vertex list, material properties)

    static void dontIntersect(int n, ...);
    static int getNextId();
    static void setNextId(int id);

    Patch(int inNumberOfVertices, Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4);

    void
    setVisible() {
        flags |= PATCH_VISIBILITY;
    }

    void
    setInvisible() {
        flags &= ~PATCH_VISIBILITY;
    }

    bool
    isVisible() const {
        return (flags & PATCH_VISIBILITY) != 0;
    }

    int hasZeroVertices() const;
    float *patchBounds(float *bounds);
    void patchPrintId(FILE *out) const;
    RayHit *intersect(Ray *ray, float minimumDistance, float *maximumDistance, int hitFlags, RayHit *hitStore);
    Vector3D *pointBarycentricMapping(double u, double v, Vector3D *point);
    Vector3D *uniformPoint(double u, double v, Vector3D *point);
    int uv(Vector3D *point, double *u, double *v);
    int uniformUv(Vector3D *point, double *u, double *v);
    void biLinearToUniform(double *u, double *v) const;
    void interpolatedFrameAtUv(double u, double v, Vector3D *X, Vector3D *Y, Vector3D *Z);
    Vector3D textureCoordAtUv(double u, double v);
    COLOR averageNormalAlbedo(BSDFFLAGS components);
    COLOR averageEmittance(XXDFFLAGS components);
};

#endif

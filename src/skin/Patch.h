#ifndef __PATCH__
#define __PATCH__

#include "common/Ray.h"
#include "skin/Jacobian.h"
#include "skin/Vertex.h"
#include "skin/MeshSurface.h"

#define MAXIMUM_VERTICES_PER_PATCH 4
#define PATCH_VISIBILITY 0x01

class Vertex;
class MeshSurface;
class Element;

class Patch {
private:
    unsigned char flags; // Other flags

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

    Patch();
    int hasZeroVertices() const;

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

    float *patchBounds(float *bounds);

    void patchPrintId(FILE *out);

    RayHit *intersect(Ray *ray, float minimumDistance, float *maximumDistance, int hitFlags, RayHit *hitStore);

    Vector3D *pointBarycentricMapping(double u, double v, Vector3D *point);

    Vector3D *uniformPoint(double u, double v, Vector3D *point);

    int uv(Vector3D *point, double *u, double *v);

    int uniformUv(Vector3D *point, double *u, double *v);

    void biLinearToUniform(double *u, double *v);

    friend Patch *
    patchCreate(int numberOfVertices, Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4);
};

extern Patch *patchCreate(int numberOfVertices, Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4);

extern void patchDontIntersect(int n, ...);
extern int patchGetNextId();
extern void patchSetNextId(int id);

extern void uniformToBiLinear(Patch *patch, double *u, double *v);
extern Vector3D patchInterpolatedNormalAtUv(Patch *patch, double u, double v);
extern void patchInterpolatedFrameAtUv(Patch *patch, double u, double v, Vector3D *X, Vector3D *Y, Vector3D *Z);
extern Vector3D patchTextureCoordAtUv(Patch *patch, double u, double v);
extern COLOR patchAverageNormalAlbedo(Patch *patch, BSDFFLAGS components);
extern COLOR patchAverageEmittance(Patch *patch, XXDFFLAGS components);

#include "skin/Element.h"

#endif

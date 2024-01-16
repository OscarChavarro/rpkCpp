#ifndef __PATCH__
#define __PATCH__

#include "common/Ray.h"
#include "BREP/BREP_FACE.h"
#include "skin/Jacobian.h"
#include "skin/Vertex.h"
#include "skin/MeshSurface.h"

#define MAXIMUM_VERTICES_PER_PATCH 4
#define PATCH_VISIBILITY 0x01
#define PATCH_IS_VISIBLE(patch) (((patch)->flags & PATCH_VISIBILITY) != 0)
#define PATCH_SET_VISIBLE(patch) {(patch)->flags |= PATCH_VISIBILITY;}
#define PATCH_SET_INVISIBLE(patch) {(patch)->flags &= ~PATCH_VISIBILITY;}

class Vertex;
class MeshSurface;

class Patch {
public:
    unsigned id; // identification number for debugging, ID rendering

    Patch *twin; // twin face (for double-sided surfaces)
    BREP_FACE *brepData; // topological data for the patch. Only filled in if a radiance method needs it

    Vertex *vertex[MAXIMUM_VERTICES_PER_PATCH]; // pointers to the vertices
    char numberOfVertices; // number of vertices: 3 or 4

    float *bounds; // bounding box

    Vector3D normal; // patch normal
    float planeConstant; // patch plane constant
    float tolerance; // patch plane tolerance
    float area; // patch area
    Vector3D midpoint; // patch midpoint
    Jacobian *jacobian; /* shape-related constants for irregular quadrilaterals.
                         Used for sampling the quadrilateral and for computing integrals
                         */
    float directPotential; /* directly received hemispherical potential
                               (ref: Pattanaik, ACM Trans Graph).
                               Only determined when asked to do so (see potential.[ch]).
                               */
    char index; // indicates dominant part of patch normal
    char omit; /* indicates that the patch should not be considered
                  for a couple of things, such as intersection
                  testing, shaft culling, ... set to FALSE by
                  default. Don't forget to set to FALSE again
			      after you changed it!! */
    unsigned char flags; // other flags

    RGB color; // color used to flat render the patch

    void *radiance_data; // data needed for radiance computations. Type depends on the current radiance algorithm
    MeshSurface *surface; // pointer to surface data (contains vertex list, material properties)

    Patch();
    int isVirtual() const;
};

extern Patch *patchCreate(int numberOfVertices, Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4);
extern void patchDestroy(Patch *patch);
extern float *patchBounds(Patch *patch, float *bounds);
extern RayHit *patchIntersect(Patch *patch, Ray *ray, float minimumDistance, float *maximumDistance, int hitFlags, RayHit *hitStore);
extern void patchDontIntersect(int n, ...);
extern void patchPrint(FILE *out, Patch *patch);
extern void patchPrintId(FILE *out, Patch *patch);
extern int patchGetNextId();
extern void patchSetNextId(int id);
extern Vector3D *patchPoint(Patch *patch, double u, double v, Vector3D *point);
extern Vector3D *patchUniformPoint(Patch *patch, double u, double v, Vector3D *point);
extern int patchUv(Patch *poly, Vector3D *point, double *u, double *v);
extern int patchUniformUv(Patch *poly, Vector3D *point, double *u, double *v);
extern void bilinearToUniform(Patch *patch, double *u, double *v);
extern void uniformToBilinear(Patch *patch, double *u, double *v);
extern Vector3D patchInterpolatedNormalAtUv(Patch *patch, double u, double v);
extern void patchInterpolatedFrameAtUv(Patch *patch, double u, double v, Vector3D *X, Vector3D *Y, Vector3D *Z);
extern Vector3D patchTextureCoordAtUv(Patch *patch, double u, double v);
extern COLOR patchAverageNormalAlbedo(Patch *patch, BSDFFLAGS components);
extern COLOR patchAverageEmittance(Patch *patch, XXDFFLAGS components);
extern int materialShadingFrame(RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);

#endif

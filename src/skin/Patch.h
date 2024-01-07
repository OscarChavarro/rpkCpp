#ifndef _PATCH_H_
#define _PATCH_H_

#include "common/Ray.h"
#include "BREP/BREP_FACE.h"
#include "skin/Jacobian.h"
#include "skin/Vertex.h"
#include "skin/MeshSurface.h"

#define MAXIMUM_VERTICES_PER_PATCH 4 // max. 4 vertices per patch
#define PATCH_VISIBILITY 0x01
#define PATCH_IS_VISIBLE(patch) (((patch)->flags & PATCH_VISIBILITY) != 0)
#define PATCH_SET_VISIBLE(patch) {(patch)->flags |= PATCH_VISIBILITY;}
#define PATCH_SET_INVISIBLE(patch) {(patch)->flags &= ~PATCH_VISIBILITY;}

class VERTEX;
class MeshSurface;

class PATCH {
public:
    unsigned id; // identification number for debugging, ID rendering

    PATCH *twin; // twin face (for double-sided surfaces)
    BREP_FACE *brep_data; // topological data for the patch. Only filled in if a radiance method needs it

    VERTEX *vertex[MAXIMUM_VERTICES_PER_PATCH]; // pointers to the vertices
    char nrvertices; // number of vertices: 3 or 4

    float *bounds; // bounding box

    Vector3D normal; // patch normal
    float plane_constant; // patch plane constant
    float tolerance; // patch plane tolerance
    float area; // patch area
    Vector3D midpoint; // patch midpoint
    Jacobian *jacobian; /* shape-related constants for irregular quadrilaterals.
                         Used for sampling the quadrilateral and for computing integrals
                         */
    float direct_potential; /* directly received hemispherical potential
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

    int isPatchVirtual();
};

extern int IsPatchVirtual(PATCH *patch);
extern PATCH *PatchCreate(int nrvertices, VERTEX *v1, VERTEX *v2, VERTEX *v3, VERTEX *v4);
extern void PatchDestroy(PATCH *patch);
extern float *PatchBounds(PATCH *patch, float *bounds);
extern HITREC *PatchIntersect(PATCH *patch, Ray *ray, float mindist, float *maxdist, int hitflags, HITREC *hitstore);
extern void PatchDontIntersect(int n, ...);
extern void PatchPrint(FILE *out, PATCH *patch);
extern void PatchPrintID(FILE *out, PATCH *patch);
extern int PatchGetNextID();
extern void PatchSetNextID(int id);
extern Vector3D *PatchPoint(PATCH *patch, double u, double v, Vector3D *point);
extern Vector3D *PatchUniformPoint(PATCH *patch, double u, double v, Vector3D *point);
extern int PatchUV(PATCH *poly, Vector3D *point, double *u, double *v);
extern int PatchUniformUV(PATCH *poly, Vector3D *point, double *u, double *v);
extern void BilinearToUniform(PATCH *patch, double *u, double *v);
extern void UniformToBilinear(PATCH *patch, double *u, double *v);
extern Vector3D PatchInterpolatedNormalAtUV(PATCH *patch, double u, double v);
extern void PatchInterpolatedFrameAtUV(PATCH *patch, double u, double v, Vector3D *X, Vector3D *Y, Vector3D *Z);
extern Vector3D PatchTextureCoordAtUV(PATCH *patch, double u, double v);
extern COLOR PatchAverageNormalAlbedo(PATCH *patch, BSDFFLAGS components);
extern COLOR PatchAverageEmittance(PATCH *patch, XXDFFLAGS components);
extern int MaterialShadingFrame(HITREC *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);

#endif

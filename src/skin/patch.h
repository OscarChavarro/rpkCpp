#ifndef _PATCH_H_
#define _PATCH_H_

#include "common/Ray.h"
#include "BREP/BREP_FACE.h"
#include "skin/jacobian.h"
#include "skin/Vertex.h"
#include "skin/surface.h"

#define PATCHMAXVERTICES 4 // max. 4 vertices per patch

class VERTEX;

class PATCH {
  public:
    unsigned id; // identification number for debugging, ID rendering

    PATCH *twin; // twin face (for double sided surfaces)
    BREP_FACE *brep_data; // topological data for the patch. Only filled in if a radiance method needs it

    VERTEX *vertex[PATCHMAXVERTICES]; // pointers to the vertices
    char nrvertices; // number of vertices: 3 or 4

    SURFACE *surface; // pointer to surface data (contains vertexlist, material properties

    float *bounds; // bounding box

    Vector3D normal; // patch normal
    float plane_constant; // patch plane constant
    float tolerance; // patch plane tolerance
    float area; // patch area
    Vector3D midpoint; // patch midpoint
    JACOBIAN *jacobian; /* shape-related constants for irregular quadrilaterals.
                         * Used for sampling the quadrilateral and for computing
                         * integrals. */
    float direct_potential; /* directly received hemispherical potential
                         * (ref: Pattanaik, ACM Trans Graph).
                         * Only determined when asked to do so
                         * (see potential.[ch]). */
    char index; // indicates dominant part of patch normal
    char omit; /* indicates that the patch should not be considered
                         * for a couple of things, such as intersection
                         * testing, shaft culling, ... set to FALSE by
                         * default. Don't forget to set to FALSE again
			 * after you changed it!! */
    unsigned char flags; // other flags

    RGB color; // color used to flat render the patch

    void *radiance_data; // data needed for radiance computations. Type depends on the current radiance algorithm
};

/* return true if patch is virtual */
extern int IsPatchVirtual(PATCH *patch);

/* Creates a patch structure for a patch with given vertices and sidesness. */
extern PATCH *PatchCreate(int nrvertices,
                          VERTEX *v1, VERTEX *v2,
                          VERTEX *v3, VERTEX *v4);

/* disposes the memory allocated for the patch, does not remove
 * the pointers to the patch in the VERTEXes of the patch. */
extern void PatchDestroy(PATCH *patch);

/* computes a bounding box for the patch. fills it in in 'bounds' and returns
 * a pointer to 'bounds'. */
extern float *
PatchBounds(PATCH *patch, float *bounds);

/* ray-patch intersection test, for computing formfactors, creating raycast 
 * images ... Returns nullptr if the Ray doesn't hit the patch. Fills in the given
 * hit record 'the_hit' and returns a pointer to it if a hit is found.
 * Fills in the distance to the patch in maxdist if the patch
 * is hit. Intersections closer than mindist or further than *maxdist are
 * ignored. The hitflags determine what information to return about an
 * intersection and whether or not front/back facing patches are to be 
 * considered. */

extern HITREC *
PatchIntersect(PATCH *patch, Ray *ray, float mindist, float *maxdist, int hitflags, HITREC *hitstore);

/* Specify up to MAX_EXCLUDED_PATCHES patches not to test for intersections with. 
 * Used to avoid selfintersections when raytracing. First argument is number of
 * patches to include. Next arguments are pointers to the patches to exclude.
 * Call with first parameter == 0 to clear the list. */
extern void PatchDontIntersect(int n, ...);

extern void PatchPrint(FILE *out, PATCH *patch);

extern void PatchPrintID(FILE *out, PATCH *patch);

/* for debugging and ID rendering: */
/* This routine returns the ID number the next patch would get */
extern int PatchGetNextID();

/* With this routine, the ID nummer is set that the next patch will get. 
 * Note that patch ID 0 is reserved. The smallest patch ID number should be 1. */
extern void PatchSetNextID(int id);

/* Given the parameter (u,v) of a point on the patch, this routine 
 * computes the 3D space coordinates of the same point, using barycentric
 * mapping for a triangle and bilinear mapping for a quadrilateral. 
 * u and v should be numbers in the range [0,1]. If u+v>1 for a triangle,
 * (1-u) and (1-v) are used instead. */
extern Vector3D *PatchPoint(PATCH *patch, double u, double v, Vector3D *point);

/* Like above, except that always a uniform mapping is used (one that
 * preserves area, with this mapping you'll have more points in "stretched"
 * regions of an irregular quadrilateral, irregular quadrilaterals are the
 * only onces for which this routine will yield other points than the above
 * routine). */
extern Vector3D *PatchUniformPoint(PATCH *patch, double u, double v, Vector3D *point);

/* computes (u,v) parameters of the point on the patch (barycentric or bilinear
 * parametrisation). Returns TRUE if the point is inside the patch and FALSE if 
 * not. 
 * WARNING: The (u,v) coordinates are correctly computed only for points inside 
 * the patch. For points outside, they can be garbage!!! */
extern int PatchUV(PATCH *poly, Vector3D *point, double *u, double *v);

/* Like above, but returns uniform coordinates (inverse of PatchUniformPoint()). */
extern int PatchUniformUV(PATCH *poly, Vector3D *point, double *u, double *v);

/* Converts bilinear to uniform coordinates and vice versa. Use this routines
 * only for patches with explicitely given jacobian! */
extern void BilinearToUniform(PATCH *patch, double *u, double *v);

extern void UniformToBilinear(PATCH *patch, double *u, double *v);

/* same, but directly given the bilinear coordinates of the point on the patch */
extern Vector3D PatchInterpolatedNormalAtUV(PATCH *patch, double u, double v);

/* computes a interpolated (shading) frame at the uv or point with
 * given parameters on the patch. The frame is consistent over the
 * complete patch if the shading normals in the vertices do not differ
 * too much from the geometric normal. The Z axis is the interpolated
 * normal The X is determined by Z and the projection of the patch by
 * the dominant axis (patch->index).  
 */
extern void PatchInterpolatedFrameAtUV(PATCH *patch, double u, double v,
                                       Vector3D *X, Vector3D *Y, Vector3D *Z);

/* returns texture coordinates determined from vertex texture coordinates and
 * given u and v bilinear of barycentric coordinates on the patch. */
extern Vector3D PatchTextureCoordAtUV(PATCH *patch, double u, double v);

/* Use next function (with PatchListIterate) to close any open
   files of the pathc use for recording */

/* Computes average scattered power and emittance of the PATCH */
extern COLOR PatchAverageNormalAlbedo(PATCH *patch, BSDFFLAGS components);

extern COLOR PatchAverageEmittance(PATCH *patch, XXDFFLAGS components);

/* Computes shading frame at hit point. Z is the shading normal. Returns FALSE
 * if the shading frame could not be determined.
 * If X and Y are null pointers, only the shading normal is returned in Z
 * possibly avoiding computations of the X and Y axis. */
extern int MaterialShadingFrame(HITREC *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);

#endif

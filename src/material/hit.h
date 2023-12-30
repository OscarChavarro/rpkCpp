/* hit.h: hit record structure, returned by ray-object intersection routines and
 * used as a parameter for BSDF/EDF queries */

#ifndef _HIT_H_
#define _HIT_H_

#include "common/linealAlgebra/vectorMacros.h"

/* information returned by ray-geometry or ray-discretisation intersection 
 * routines */

// TODO SITHMASTER: This is coupling hit with scene level classes :(
class GEOM;
class PATCH;
class MATERIAL;

class HITREC {
  public:
    GEOM *geom; // geometry that was hit
    PATCH *patch; // patch that was hit
    Vector3D point; // intersection point
    Vector3D gnormal; // geometric normal
    MATERIAL *material; // material of hit surface
    Vector3D normal; // shading normal
    Vector3D texCoord; // texture coordinate
    Vector3D X; // shading frame (Z = shading normal: hit->Z == hit->normal)
    Vector3D Y;
    Vector3D Z;
    Vector2Dd uv; // bilinear/barycentric parameters of hit
    float dist; // distance to ray origin: always computed
    unsigned int flags; // flags indicating which of the above fields have been filled in
};

/*
The flags below have a double function: if passed as an argument 
to a ray intersection routine, they indicate that only front or back 
or both kind of intersections should be returned.
On output, they contain whether a particular hit record returned by
a ray itnersection routine is a front or back hit.
*/
#define HIT_FRONT 0x10000 // return intersections with surfaces oriented towards the origin of the ray
#define HIT_BACK  0x20000 // return intersections with surfaces oriented away from the origin of the ray
#define HIT_ANY   0x40000 // return any intersection point, not necessarily the nearest one. Used for shadow rays e.g.

/* The following flags indicate what fields are available in a hit record */

/* These flags are set by ray intersection routines */
#define HIT_GEOM     0x01 // intersected GEOM (currently never set)
#define HIT_PATCH    0x02 // intersected PATCH (returned by DiscretisationHit routines)
#define HIT_POINT    0x04 // intersection point
#define HIT_GNORMAL  0x08 // geometric normal
#define HIT_MATERIAL 0x10 // material properties at intersection point
#define HIT_DIST     0x20 // distance to hit along the ray

/* These flags are only set by the routines HitUV() etc... below */
#define HIT_UV           0x100 // (u,v) parameters (filled in by HitUV() routine below)
#define HIT_TEXCOORD     0x200 // texture coordinates (filled in by HitTexCoord()) 
#define HIT_SHADINGFRAME 0x400 /* shading frame (filled in by HitShadingFrame()).
                                         * The Z axis of the shading frame is the shading 
                                         * normal and may differ from the geometric normal */
#define HIT_NORMAL       0x800 // shading normal (filled in by HitShadingNormal() or HitShadingFrame())

/* Initialises a hit record. Either patch or geom shall be non-null. Returns
 * TRUE if the structure is properly initialised and FALSE if not. 
 * This routine can be used in order to construct BSDF queries at other points
 * than hit points returned by ray intersection routines. */
extern int InitHit(
    HITREC *hit,
    PATCH *patch,
    GEOM *geom,
    Vector3D *point,
    Vector3D *gnormal,
    MATERIAL *material,
    float dist);

/* Checks whether or not the hit record is properly initialised, that
 * means that at least 'patch' or 'geom' plus 'point', 'gnormal', 'material'
 * and 'dist' are initialised. Returns TRUE if the structure is properly
 * initialised and FALSE if not. */
extern int HitInitialised(HITREC *hit);

/* Fills in (u,v) paramters of hit point on the hit patch, computing it if not 
 * computed before. Returns FALSE if the (u,v) parameters could not be determined. */
extern int HitUV(HITREC *hit, Vector2Dd *uv);

/* Fills in/computes texture coordinates of hit point */
extern int HitTexCoord(HITREC *hit, Vector3D *texCoord);

/* Fills in shading frame: Z is the shading normal. */
extern int HitShadingFrame(HITREC *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);

/* Fills in shading normal (Z axis of shading frame) only, avoiding computation
 * of shading X and Y axis if possible */
extern int HitShadingNormal(HITREC *hit, Vector3D *normal);

#endif

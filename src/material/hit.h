/**
Hit record structure, returned by ray-object intersection routines and
used as a parameter for BSDF/EDF queries
*/

#ifndef __HIT__
#define __HIT__

#include "common/linealAlgebra/Vector2Dd.h"
#include "common/linealAlgebra/Vector3D.h"

// TODO SITHMASTER: This is coupling hit with scene level classes :(
class Geometry;
class Patch;
class Material;

/**
Information returned by ray-geometry or ray-discretization intersection
routines
*/
class RayHit {
  public:
    Geometry *geom; // geometry that was hit
    Patch *patch; // patch that was hit
    Vector3D point; // intersection point
    Vector3D gnormal; // geometric normal
    Material *material; // material of hit surface
    Vector3D normal; // shading normal
    Vector3D texCoord; // texture coordinate
    Vector3D X; // shading frame (Z = shading normal: hit->Z == hit->normal)
    Vector3D Y;
    Vector3D Z;
    Vector2Dd uv; // Bi-linear/barycentric parameters of hit
    float dist; // distance to ray origin: always computed
    unsigned int flags; // flags indicating which of the above fields have been filled in
};

/**
The flags below have a double function: if passed as an argument 
to a ray intersection routine, they indicate that only front or back 
or both kind of intersections should be returned.
On output, they contain whether a particular hit record returned by
a ray intersection routine is a front or back hit
*/
#define HIT_FRONT 0x10000 // return intersections with surfaces oriented towards the origin of the ray
#define HIT_BACK  0x20000 // return intersections with surfaces oriented away from the origin of the ray
#define HIT_ANY   0x40000 // return any intersection point, not necessarily the nearest one. Used for shadow rays e.g.

/* The following flags indicate what fields are available in a hit record */

/* These flags are set by ray intersection routines */
#define HIT_GEOM     0x01 // intersected Geometry (currently never set)
#define HIT_PATCH    0x02 // intersected Patch (returned by DiscretisationHit routines)
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

extern int
hitInit(
    RayHit *hit,
    Patch *patch,
    Geometry *geom,
    Vector3D *point,
    Vector3D *gNormal,
    Material *material,
    float dist);

extern int hitInitialised(RayHit *hit);
extern int hitUv(RayHit *hit, Vector2Dd *uv);
extern int hitTexCoord(RayHit *hit, Vector3D *texCoord);
extern int hitShadingFrame(RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);
extern int hitShadingNormal(RayHit *hit, Vector3D *normal);
extern int hitPointShadingFrame(RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);

#endif

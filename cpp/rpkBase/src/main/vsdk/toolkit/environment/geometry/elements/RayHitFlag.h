#ifndef __RAY_HIT_FLAG__
#define __RAY_HIT_FLAG__

/**
The flags below have a double function: if passed as an argument
to a ray intersection routine, they indicate that only front or back
or both kind of intersections should be returned.
On output, they contain whether a particular hit record returned by
a ray intersection routine is a front or back hit
*/
enum RayHitFlag {
    // The following flags indicate what fields are available in a hit record
    // These flags are set by ray intersection routines
    GEOMETRY = 0x01, // Intersected Geometry
    PATCH = 0x02, // Intersected Patch (returned by discretizationHit routines)
    POINT = 0x04, // Intersection point
    GEOMETRIC_NORMAL = 0x08, // Geometric normal
    MATERIAL = 0x10, // Material properties at intersection point
    DISTANCE = 0x20, // Distance to hit along the ray

    // These flags are only set by the routines HitUV() etc.
    UV = 0x100, // (u,v) parameters (filled in by HitUV() routine below)
    TEXTURE_COORDINATE = 0x200, // Texture coordinates
    SHADING_FRAME = 0x400, // Shading frame (filled in by HitShadingFrame())

    // The Z axis of the shading frame is the shading
    // normal and may differ from the geometric normal
    NORMAL = 0x800, // shading normal (filled in by HitShadingNormal() or HitShadingFrame())

    FRONT = 0x10000, // Return intersections with surfaces oriented towards the origin of the ray
    BACK = 0x20000, // Return intersections with surfaces oriented away from the origin of the ray
    ANY = 0x40000, // Return any intersection point, not necessarily the nearest one. Used for shadow rays e.g.
};

#endif

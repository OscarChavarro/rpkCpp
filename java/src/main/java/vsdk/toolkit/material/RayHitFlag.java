package vsdk.toolkit.material;

/**
The flags below have a double function: if passed as an argument
to a ray intersection routine, they indicate that only front or back
or both kind of intersections should be returned.
On output, they contain whether a particular hit record returned by
a ray intersection routine is a front or back hit
*/

// The following flags indicate what fields are available in a hit record

// These flags are set by ray intersection routines

// Intersected Geometry

// Intersected Patch (returned by discretizationHit routines)

// Intersection point

// Geometric normal

// Material properties at intersection point

// Distance to hit along the ray

// These flags are only set by the routines HitUV() etc.

// (u,v) parameters (filled in by HitUV() routine below)

// Texture coordinates

// Shading frame (filled in by HitShadingFrame())

// The Z axis of the shading frame is the shading

// normal and may differ from the geometric normal

// shading normal (filled in by HitShadingNormal() or HitShadingFrame())

// Return intersections with surfaces oriented towards the origin of the ray

// Return intersections with surfaces oriented away from the origin of the ray

// Return any intersection point, not necessarily the nearest one. Used for shadow rays e.g.

public final class RayHitFlag {
    public static final int GEOMETRY = 0x01;
    public static final int PATCH = 0x02;
    public static final int POINT = 0x04;
    public static final int GEOMETRIC_NORMAL = 0x08;
    public static final int MATERIAL = 0x10;
    public static final int DISTANCE = 0x20;

    public static final int UV = 0x100;
    public static final int TEXTURE_COORDINATE = 0x200;
    public static final int SHADING_FRAME = 0x400;

    public static final int NORMAL = 0x800;

    public static final int FRONT = 0x10000;
    public static final int BACK = 0x20000;
    public static final int ANY = 0x40000;

    private RayHitFlag() {
    }
}

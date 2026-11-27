package vsdk.toolkit.environment.geometry.element;

import vsdk.toolkit.environment.geometry.Geometry;

/**
Source compatibility: the boundary value is also valid for open surfaces.
cpp declares `typedef RayBoundary RaySolidBoundary`; Java has no type alias,
so this subclass adds nothing and RaySolidClassification stores RayBoundary.
*/
public class RaySolidBoundary extends RayBoundary {
    public RaySolidBoundary() { super(); }
    public RaySolidBoundary(RayBoundary other) { super(other); }
    public RaySolidBoundary(double t, Geometry geometry) { super(t, geometry); }
    public RaySolidBoundary(double t, Geometry geometry, boolean normalReversed) { super(t, geometry, normalReversed); }
    public RaySolidBoundary(double t, Geometry geometry, boolean normalReversed, long surfaceIndex) { super(t, geometry, normalReversed, surfaceIndex); }
}

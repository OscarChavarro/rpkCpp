package vsdk.toolkit.environment.geometry.element;

import vsdk.toolkit.environment.geometry.Geometry;

/** Mathematical provenance only. Scene resolves appearance from the borrowed leaf. */
public class RayBoundary {
    public double t;
    public Geometry geometry;
    public boolean normalReversed;
    /** Face/element within the leaf geometry, or -1 when not applicable. */
    public long surfaceIndex;
    // Ordered from the leaf instance outward. Values are snapshots, identities borrowed.
    private final RayInstancePath path;

    public RayInstancePath getProvenance() { return path; }
    public boolean appendInstance(RayInstanceStep step) { return path.appendInstance(step); }
    public long getInstanceCount() { return path.getInstanceCount(); }
    public boolean getInstance(long index, RayInstanceStep result) { return path.getInstance(index, result); }
    public void clearInstances() { path.clearInstances(); }

    public RayBoundary() { t = 0; geometry = null; normalReversed = false; surfaceIndex = -1; path = new RayInstancePath(); }
    public RayBoundary(RayBoundary other)
    {
        t = other.t; geometry = other.geometry; normalReversed = other.normalReversed;
        surfaceIndex = other.surfaceIndex; path = new RayInstancePath(other.path);
    }
    /** cpp copy assignment. */
    public RayBoundary assign(RayBoundary other)
    {
        if (this != other) { t = other.t; geometry = other.geometry; normalReversed = other.normalReversed; surfaceIndex = other.surfaceIndex; path.assign(other.path); }
        return this;
    }
    public RayBoundary(double t, Geometry geometry) { this(t, geometry, false, -1); }
    public RayBoundary(double t, Geometry geometry, boolean normalReversed) { this(t, geometry, normalReversed, -1); }
    public RayBoundary(double value, Geometry leaf, boolean reversed, long index)
    { t = value; geometry = leaf; normalReversed = reversed; surfaceIndex = index; path = new RayInstancePath(); }
}

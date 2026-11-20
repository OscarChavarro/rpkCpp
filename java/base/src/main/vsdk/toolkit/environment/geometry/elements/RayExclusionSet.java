package vsdk.toolkit.environment.geometry.elements;

import vsdk.toolkit.skin.Geometry;

/**
Patches and geometries that a visibility query must ignore, typically the
source and receiver of a form factor (and their twins), to avoid immediate
self-intersections. The value is built by the caller and passed down every
intersect call, so two queries never see each other's exclusions.

Shared, read-only: it has no setters, and every query is const.
*/
public final class RayExclusionSet {
    private final Patch excludedPatch0;
    private final Patch excludedPatch1;
    private final Patch excludedPatch2;
    private final Patch excludedPatch3;
    private final Geometry excludedGeometry1;
    private final Geometry excludedGeometry2;

    public RayExclusionSet() {
        this(null, null, null, null, null, null);
    }

    public RayExclusionSet(Patch p0, Patch p1) {
        this(p0, p1, null, null, null, null);
    }

    public RayExclusionSet(Patch p0, Patch p1, Patch p2) {
        this(p0, p1, p2, null, null, null);
    }

    public RayExclusionSet(
        Patch p0,
        Patch p1,
        Patch p2,
        Patch p3,
        Geometry geometry1,
        Geometry geometry2) {
        excludedPatch0 = p0;
        excludedPatch1 = p1;
        excludedPatch2 = p2;
        excludedPatch3 = p3;
        excludedGeometry1 = geometry1;
        excludedGeometry2 = geometry2;
    }

    public boolean excludesPatch(Patch patch) {
        return excludedPatch0 == patch || excludedPatch1 == patch || excludedPatch2 == patch || excludedPatch3 == patch;
    }

    public boolean excludesGeometry(Geometry geometry) {
        return geometry == excludedGeometry1 || geometry == excludedGeometry2;
    }
}

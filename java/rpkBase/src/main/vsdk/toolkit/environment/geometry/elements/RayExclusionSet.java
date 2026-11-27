package vsdk.toolkit.environment.geometry.elements;

import vsdk.toolkit.skin.RpkGeometry;

/**
Patches and geometries that a visibility query must ignore, typically the
source and receiver of a form factor (and their twins), to avoid immediate
self-intersections. The value is built by the caller and passed down every
intersect call, so two queries never see each other's exclusions.

An excluded geometry (a source or receiver cluster) excludes every face
below it, wherever the spatial index stores that face: as part of the
geometry, of a sub-geometry, or as a loose patch. Since faces are numbered
depth-first over the cluster hierarchy (Scene.buildFaceTable()), the faces
below a geometry are a contiguous interval of face indices, so the face test
is two comparisons per excluded geometry and needs no pointers.

Shared, read-only: it has no setters, and every query is const.
*/
public final class RayExclusionSet {
    public static final int MAX_EXCLUDED_PATCHES = 4;

    private final Patch excludedPatch0;
    private final Patch excludedPatch1;
    private final Patch excludedPatch2;
    private final Patch excludedPatch3;
    private final RpkGeometry excludedGeometry1;
    private final RpkGeometry excludedGeometry2;
    // Face index intervals of the excluded geometries, empty when first > last
    private final int excludedFirstFace1;
    private final int excludedLastFace1;
    private final int excludedFirstFace2;
    private final int excludedLastFace2;

    public RayExclusionSet() {
        this(null, null, null, null, null, null);
    }

    public RayExclusionSet(Patch p0, Patch p1) {
        this(p0, p1, null, null, null, null);
    }

    public RayExclusionSet(Patch p0, Patch p1, Patch p2) {
        this(p0, p1, p2, null, null, null);
    }

    /**
    Excludes up to four patches and up to two geometries. The face index
    intervals of the geometries are taken at construction time, so the face
    table must have been built (Scene.buildFaceTable()) before
    */
    public RayExclusionSet(
        Patch p0,
        Patch p1,
        Patch p2,
        Patch p3,
        RpkGeometry geometry1,
        RpkGeometry geometry2) {
        excludedPatch0 = p0;
        excludedPatch1 = p1;
        excludedPatch2 = p2;
        excludedPatch3 = p3;
        excludedGeometry1 = geometry1;
        excludedGeometry2 = geometry2;
        excludedFirstFace1 = geometry1 != null ? geometry1.getFirstFaceIndex() : 0;
        excludedLastFace1 = geometry1 != null ? geometry1.getLastFaceIndex() : -1;
        excludedFirstFace2 = geometry2 != null ? geometry2.getFirstFaceIndex() : 0;
        excludedLastFace2 = geometry2 != null ? geometry2.getLastFaceIndex() : -1;
    }

    public boolean excludesPatch(Patch patch) {
        return excludedPatch0 == patch || excludedPatch1 == patch || excludedPatch2 == patch || excludedPatch3 == patch;
    }

    public boolean excludesGeometry(RpkGeometry geometry) {
        return geometry == excludedGeometry1 || geometry == excludedGeometry2;
    }

    /**
    True when the face lies below one of the excluded geometries
    */
    public boolean excludesFace(int faceIndex) {
        return (faceIndex >= excludedFirstFace1 && faceIndex <= excludedLastFace1)
            || (faceIndex >= excludedFirstFace2 && faceIndex <= excludedLastFace2);
    }

    public Patch getExcludedPatch(int index) {
        switch (index) {
            case 0:
                return excludedPatch0;
            case 1:
                return excludedPatch1;
            case 2:
                return excludedPatch2;
            case 3:
                return excludedPatch3;
            default:
                throw new IndexOutOfBoundsException(index);
        }
    }

    public RpkGeometry getExcludedGeometry1() {
        return excludedGeometry1;
    }

    public RpkGeometry getExcludedGeometry2() {
        return excludedGeometry2;
    }
}

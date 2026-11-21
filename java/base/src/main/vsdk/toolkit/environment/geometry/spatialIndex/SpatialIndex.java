package vsdk.toolkit.environment.geometry.spatialIndex;

import vsdk.toolkit.environment.geometry.element.Ray;

/**
A spatial index over bounded primitives identified by dense ids 0 .. n - 1.
It only prunes: the caller's `PrimitiveRayTest` decides every hit, so every
conservative index returns the same visibility for every ray, even when two
indexes report different blockers.

Built once from conservative primitive boxes (the caller pads them so that
they contain whatever its exact test may accept); geometry or placement
changes are handled by building a new index. A query writes only to the
workspace and the test it is given, so any number of workers may share one
index, each with its own workspace and test.

Rays are affine (`Ray.fromAffine`): the parameter t is the one of
origin + t * direction, as in the caller's exact test.
*/
public abstract class SpatialIndex {
    public abstract int getNumberOfPrimitives();

    /**
    Size of the workspace mailbox this index needs (zero when it uses none);
    give it to the `SpatialIndexWorkspace` constructor.
    */
    public abstract int getWorkspaceSize();

    /**
    Tests, with the caller's exact test, the primitives the ray may hit with
    a parameter in [minimumT, maximumT].
    @param minimumT inclusive, not negative; the index does not prune by it,
    the test applies it
    @return the accepted hit: the closest one (CLOSEST_HIT) or the first one
    found (ANY_HIT), or no primitive
    */
    public abstract SpatialQueryResult query(
        Ray ray,
        double minimumT,
        double maximumT,
        SpatialQueryMode mode,
        PrimitiveRayTest test,
        SpatialIndexWorkspace workspace);
}

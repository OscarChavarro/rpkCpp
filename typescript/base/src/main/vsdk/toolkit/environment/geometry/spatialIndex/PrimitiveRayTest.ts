/**
Exact test of one primitive of a `SpatialIndex`, supplied by the caller of a
query. The index only prunes: it calls `test` for every primitive the ray may
reach and the test decides. It applies the minimum parameter, the caller's
exclusions (an excluded primitive is rejected before it can shorten the
interval) and the tie rule between hits at equal parameters.

A test object belongs to one query, or to one worker: it may keep the detail
of the accepted hit (point, face coordinates, normal) for the caller, so it is
not shared between concurrent queries.
*/
export interface PrimitiveRayTest {
    /**
    @param primitive id of the primitive, as given when the index was built
    @param minimumT smallest accepted ray parameter (inclusive)
    @param maximumT largest accepted ray parameter: the query interval end,
    or the parameter of the hit accepted so far in a closest-hit query
    @param hitT one-element holder (C++ `double *`): receives the parameter
    of the hit when it is accepted
    @return true when the ray hits the primitive within the interval and the
    hit is accepted
    */
    test(primitive: number, minimumT: number, maximumT: number, hitT: number[]): boolean;
}

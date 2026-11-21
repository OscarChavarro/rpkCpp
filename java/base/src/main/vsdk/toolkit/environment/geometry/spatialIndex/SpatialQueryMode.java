package vsdk.toolkit.environment.geometry.spatialIndex;

/**
What a `SpatialIndex` query looks for:
- CLOSEST_HIT: every candidate the ray may reach before the closest accepted
  hit is tested, and the closest accepted hit is returned;
- ANY_HIT: the query ends at the first accepted hit (visibility, shadow
  rays); which blocker is returned depends on the index.
*/
public enum SpatialQueryMode {
    CLOSEST_HIT,
    ANY_HIT
}

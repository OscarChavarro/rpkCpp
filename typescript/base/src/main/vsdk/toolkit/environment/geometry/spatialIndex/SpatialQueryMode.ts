/**
What a `SpatialIndex` query looks for:
- CLOSEST_HIT: every candidate the ray may reach before the closest accepted
  hit is tested, and the closest accepted hit is returned;
- ANY_HIT: the query ends at the first accepted hit (visibility, shadow
  rays); which blocker is returned depends on the index.
*/
export enum SpatialQueryMode {
  CLOSEST_HIT = 0,
  ANY_HIT = 1
}

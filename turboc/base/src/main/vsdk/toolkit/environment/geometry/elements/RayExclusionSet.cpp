#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/environment/geometry/elements/RayExclusionSet.h"

/**
Excludes up to four patches and up to two geometries. The face index
intervals of the geometries are taken at construction time, so the face
table must have been built (Scene::buildFaceTable()) before
*/
RayExclusionSet::RayExclusionSet(
    const Patch *p0,
    const Patch *p1,
    const Patch *p2,
    const Patch *p3,
    const Geometry *geometry1,
    const Geometry *geometry2):
    excludedPatch(),
    excludedGeometry1(geometry1),
    excludedGeometry2(geometry2),
    excludedFirstFace1(geometry1 != NULL ? geometry1->getFirstFaceIndex() : 0),
    excludedLastFace1(geometry1 != NULL ? geometry1->getLastFaceIndex() : -1),
    excludedFirstFace2(geometry2 != NULL ? geometry2->getFirstFaceIndex() : 0),
    excludedLastFace2(geometry2 != NULL ? geometry2->getLastFaceIndex() : -1)
{
    excludedPatch[0] = p0;
    excludedPatch[1] = p1;
    excludedPatch[2] = p2;
    excludedPatch[3] = p3;
}

#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/SceneFaceRayTest.h"

SceneFaceRayTest::SceneFaceRayTest(
    const Scene &inScene,
    const RpkRay *inRay,
    const int inHitFlags,
    const RayExclusionSet &inExclusions,
    RayHit *inHitStore):
    scene(inScene),
    ray(inRay),
    hitFlags(inHitFlags),
    exclusions(inExclusions),
    hitStore(inHitStore)
{
}

SceneFaceRayTest::~SceneFaceRayTest() {
}

BOOLEAN
SceneFaceRayTest::test(const int primitive, const double minimumT, const double maximumT, double *hitT) {
    float distance = ((float)(maximumT));
    const RayHit *hit = scene.getFace(primitive)->intersect(
        ray, ((float)(minimumT)), &distance, hitFlags, hitStore, exclusions);
    if ( hit == NULL ) {
        return FALSE;
    }
    *hitT = distance;
    return TRUE;
}

#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/SceneFaceRayTest.h"

SceneFaceRayTest::SceneFaceRayTest(
    const Scene &inScene,
    const RpkRay *inRay,
    const int inHitFlags,
    const RayExclusionSet &inExclusions,
    RpkRayHit *inHitStore):
    scene(inScene),
    ray(inRay),
    hitFlags(inHitFlags),
    exclusions(inExclusions),
    hitStore(inHitStore)
{
}

SceneFaceRayTest::~SceneFaceRayTest() {
}

bool
SceneFaceRayTest::test(const int primitive, const double minimumT, const double maximumT, double *hitT) {
    float distance = static_cast<float>(maximumT);
    const RpkRayHit *hit = scene.getFace(primitive)->intersect(
        ray, static_cast<float>(minimumT), &distance, hitFlags, hitStore, exclusions);
    if ( hit == nullptr ) {
        return false;
    }
    *hitT = distance;
    return true;
}

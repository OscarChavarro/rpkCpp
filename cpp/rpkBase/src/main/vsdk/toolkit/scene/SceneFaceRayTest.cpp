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

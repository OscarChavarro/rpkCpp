#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "RpkFaceRayTest.h"

RpkFaceRayTest::RpkFaceRayTest(
    const Scene &inScene,
    const FaceAncestry &inAncestry,
    RpkRay *inRay,
    const int inHitFlags,
    const RayExclusionSet &inExclusions):
    scene(inScene),
    ancestry(inAncestry),
    ray(inRay),
    hitFlags(inHitFlags),
    exclusions(inExclusions),
    hitStore()
{
}

RpkFaceRayTest::~RpkFaceRayTest() {
}

bool
RpkFaceRayTest::test(const int primitive, const double minimumT, const double maximumT, double *hitT) {
    if ( ancestry.excludesFace(primitive, exclusions) ) {
        return false;
    }
    float distance = static_cast<float>(maximumT);
    const RpkRayHit *hit = scene.getFace(primitive)->intersect(
        ray, static_cast<float>(minimumT), &distance, hitFlags, &hitStore, exclusions);
    if ( hit == nullptr ) {
        return false;
    }
    *hitT = distance;
    return true;
}

#ifndef __SCENE_FACE_RAY_TEST__
#define __SCENE_FACE_RAY_TEST__

#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/elements/RayExclusionSet.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkRayHit.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/PrimitiveRayTest.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/Scene.h"

/**
RenderPark's exact face test as the PrimitiveRayTest of a VITRAL
SpatialIndex: primitive i is face i of the scene's face table, tested with
Patch::intersect on the original ray, flags and exclusions, so an index
answer is decided by the same arithmetic as the voxel grid's, the exclusions
included (a face below an excluded geometry is rejected by its face-index
range, plan decision D5). The interval ends and the accepted hit distance are
doubles, as the face test computes them.

One object per query: it writes the accepted hit into the caller's hit
record, so after a closest-hit query the record holds the closest face.
*/
class SceneFaceRayTest final : public PrimitiveRayTest {
  private:
    const Scene &scene;
    const Ray *ray;
    int hitFlags;
    const RayExclusionSet &exclusions;
    RpkRayHit *hitStore;

  public:
    SceneFaceRayTest(
        const Scene &inScene,
        const Ray *inRay,
        int inHitFlags,
        const RayExclusionSet &inExclusions,
        RpkRayHit *inHitStore);
    ~SceneFaceRayTest() final;

    bool test(int primitive, double minimumT, double maximumT, double *hitT) final;
};

// Inline, so an index query templated on this class (GridSpatialIndex::
// queryWith) calls Patch::intersect directly
inline bool
SceneFaceRayTest::test(const int primitive, const double minimumT, const double maximumT, double *hitT) {
    double distance = static_cast<double>(maximumT);
    const RpkRayHit *hit = scene.getFace(primitive)->intersect(
        ray, static_cast<double>(minimumT), &distance, hitFlags, hitStore, exclusions);
    if ( hit == nullptr ) {
        return false;
    }
    *hitT = distance;
    return true;
}

#endif

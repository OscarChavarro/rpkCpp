#ifndef RPK_FACE_RAY_TEST__
#define RPK_FACE_RAY_TEST__

#include "vsdk/toolkit/common/linealAlgebra/RpkRay.h"
#include "vsdk/toolkit/environment/geometry/elements/RayExclusionSet.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkRayHit.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/PrimitiveRayTest.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "FaceAncestry.h"

/**
RenderPark's exact face test as the PrimitiveRayTest of a VITRAL
SpatialIndex: primitive i is face i of the scene's face table, tested with
Patch::intersect on the original single precision ray, flags and exclusions,
so an index answer is decided by the same arithmetic as the voxel grid's.
A face below an excluded geometry is rejected first (FaceAncestry).
The interval ends arrive as doubles that hold float values, and the accepted
hit distance leaves as the float the face test produced.

One object per query and worker: it keeps the accepted hit.
*/
class RpkFaceRayTest final : public PrimitiveRayTest {
  private:
    const Scene &scene;
    const FaceAncestry &ancestry;
    RpkRay *ray;
    int hitFlags;
    const RayExclusionSet &exclusions;
    RpkRayHit hitStore;

  public:
    RpkFaceRayTest(const Scene &inScene, const FaceAncestry &inAncestry, RpkRay *inRay, int inHitFlags, const RayExclusionSet &inExclusions);
    ~RpkFaceRayTest() final;

    bool test(int primitive, double minimumT, double maximumT, double *hitT) final;
};

#endif

#ifndef SCENE_QUERY__
#define SCENE_QUERY__

#include "vsdk/toolkit/common/linealAlgebra/RpkRay.h"
#include "vsdk/toolkit/environment/geometry/elements/RayExclusionSet.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkRayHit.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialIndex.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialIndexWorkspace.h"
#include "vsdk/toolkit/scene/SceneAccelerationMethod.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
#include "vsdk/toolkit/scene/VoxelGridWorkspace.h"

class Scene;
class SceneQueryWorkspace;
class VisibilityQueryRecorder;

/**
The scene's visibility query: the nearest (or, with RayHitFlag::ANY, some)
face that a ray hits within an interval, through the spatial index chosen by
a SceneAccelerationMethod. Faces are the entries of the scene's face table;
the exact face test, flags and exclusions are RenderPark's Patch::intersect
for every method, so the methods differ only in which faces they test.
An ANY query may return another blocker with each method; its visibility
answer is the same.

Shared, read-only after construction: intersect is const and writes only to
the hit record and the workspace it is given. Built after the face table;
a change of the scene geometry requires a new query.
*/
class SceneQuery final {
  private:
    const Scene *scene;
    SceneAccelerationMethod method;
    VoxelGrid *legacyGrid; // Owned; LEGACY_VOXEL_ACCELERATION only
    SpatialIndex *index; // Owned; the other methods

  public:
    SceneQuery(const Scene *inScene, SceneAccelerationMethod inMethod);
    ~SceneQuery();
    SceneQuery(const SceneQuery &) = delete;
    SceneQuery &operator=(const SceneQuery &) = delete;

    /**
    The VITRAL index of a method over the scene's face table, primitive i
    being face i, or null for LEGACY_VOXEL_ACCELERATION. The caller owns it.
    WALD2007 gets the face boxes padded by the face tolerance plus a relative
    margin, so that each contains every point the face test may accept; the
    grid gets the unpadded boxes (it pads them as the legacy grid does) and
    the cluster hierarchy as its tree of groups.
    */
    static SpatialIndex *buildIndex(const Scene &scene, SceneAccelerationMethod method);

    /** The option name of a method: legacy, voxel, wald2007, bruteforce */
    static const char *methodName(SceneAccelerationMethod method);

    /** The method of an option name; false when the name is none of them */
    static bool methodFromName(const char *name, SceneAccelerationMethod *method);

    inline SceneAccelerationMethod getMethod() const;
    inline const VoxelGrid *getLegacyGrid() const;
    inline const SpatialIndex *getIndex() const;

    /**
    As RenderPark's VoxelGrid::gridIntersect: returns hitStore filled with the
    accepted hit and sets *maximumDistance to its distance, or returns null
    and leaves both unchanged when no face is hit in
    [minimumDistance, *maximumDistance]
    */
    RpkRayHit *intersect(
        RpkRay *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions,
        SceneQueryWorkspace *workspace) const;
};

/**
Per-query scratch of a SceneQuery: the workspace of its index (or of the
legacy grid) and, in builds configured WITH_RAY_CAPTURE, the recorder the
callers report each query to (null otherwise).

Workspace: mutable, owned by one caller and passed explicitly on every
SceneQuery::intersect call.
*/
class SceneQueryWorkspace final {
  private:
    VoxelGridWorkspace *legacyWorkspace; // Owned; legacy grid only
    SpatialIndexWorkspace *indexWorkspace; // Owned; the other methods
    VisibilityQueryRecorder *recorder; // Not owned

  public:
    explicit SceneQueryWorkspace(const SceneQuery &query, VisibilityQueryRecorder *inRecorder = nullptr);
    ~SceneQueryWorkspace();
    SceneQueryWorkspace(const SceneQueryWorkspace &) = delete;
    SceneQueryWorkspace &operator=(const SceneQueryWorkspace &) = delete;

    inline VoxelGridWorkspace *getLegacyWorkspace() const;
    inline SpatialIndexWorkspace *getIndexWorkspace() const;
    inline VisibilityQueryRecorder *getRecorder() const;
};

inline SceneAccelerationMethod
SceneQuery::getMethod() const {
    return method;
}

inline const VoxelGrid *
SceneQuery::getLegacyGrid() const {
    return legacyGrid;
}

inline const SpatialIndex *
SceneQuery::getIndex() const {
    return index;
}

inline VoxelGridWorkspace *
SceneQueryWorkspace::getLegacyWorkspace() const {
    return legacyWorkspace;
}

inline SpatialIndexWorkspace *
SceneQueryWorkspace::getIndexWorkspace() const {
    return indexWorkspace;
}

inline VisibilityQueryRecorder *
SceneQueryWorkspace::getRecorder() const {
    return recorder;
}

#endif

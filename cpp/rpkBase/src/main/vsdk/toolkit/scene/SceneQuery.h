#ifndef __SCENE_QUERY__
#define __SCENE_QUERY__

#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/elements/RayExclusionSet.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkRayHit.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialIndex.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialIndexWorkspace.h"
#include "vsdk/toolkit/scene/SceneAccelerationMethod.h"

class Scene;
class SceneQueryWorkspace;

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
    SpatialIndex *index; // Owned

  public:
    SceneQuery(const Scene *inScene, SceneAccelerationMethod inMethod);
    ~SceneQuery();
    SceneQuery(const SceneQuery &) = delete;
    SceneQuery &operator=(const SceneQuery &) = delete;

    /**
    The VITRAL index of a method over the scene's face table, primitive i
    being face i. The caller owns it.
    WALD2007 gets the face boxes padded by the face tolerance plus a relative
    margin, so that each contains every point the face test may accept; the
    grid gets the unpadded boxes (it pads them as RenderPark's grid did) and
    the cluster hierarchy as its tree of groups.
    */
    static SpatialIndex *buildIndex(const Scene &scene, SceneAccelerationMethod method);

    /** The option name of a method: voxel, wald2007, bruteforce */
    static const char *methodName(SceneAccelerationMethod method);

    /** The method of an option name; false when the name is none of them */
    static bool methodFromName(const char *name, SceneAccelerationMethod *method);

    inline SceneAccelerationMethod getMethod() const;
    inline const SpatialIndex *getIndex() const;

    /**
    As RenderPark's original grid query: returns hitStore filled with the
    accepted hit and sets *maximumDistance to its distance, or returns null
    and leaves both unchanged when no face is hit in
    [minimumDistance, *maximumDistance]
    */
    RpkRayHit *intersect(
        Ray *ray,
        double minimumDistance,
        double *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions,
        SceneQueryWorkspace *workspace) const;
};

/**
Per-query scratch of a SceneQuery: the workspace of its index and an optional
ray counter.

Workspace: mutable, owned by one caller and passed explicitly on every
SceneQuery::intersect call.
*/
class SceneQueryWorkspace final {
  private:
    SpatialIndexWorkspace *indexWorkspace; // Owned
    long *rayCount = nullptr; // Optional counter owned by the caller

  public:
    explicit SceneQueryWorkspace(const SceneQuery &query);
    ~SceneQueryWorkspace();
    SceneQueryWorkspace(const SceneQueryWorkspace &) = delete;
    SceneQueryWorkspace &operator=(const SceneQueryWorkspace &) = delete;

    inline SpatialIndexWorkspace *getIndexWorkspace() const;
    void setRayCount(long *counter) { rayCount = counter; }
    long *getRayCount() const { return rayCount; }
};

inline SceneAccelerationMethod
SceneQuery::getMethod() const {
    return method;
}

inline const SpatialIndex *
SceneQuery::getIndex() const {
    return index;
}

inline SpatialIndexWorkspace *
SceneQueryWorkspace::getIndexWorkspace() const {
    return indexWorkspace;
}

#endif

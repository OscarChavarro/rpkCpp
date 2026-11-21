#ifndef VISIBILITY_QUERY_RECORDER__
#define VISIBILITY_QUERY_RECORDER__

#include <cstdio>
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/RpkRay.h"
#include "vsdk/toolkit/environment/geometry/elements/RayExclusionSet.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkRayHit.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"

/**
Ray accounting and ray corpus capture of the visibility queries of one run.
Only builds configured with WITH_RAY_CAPTURE (RPK_RAY_CAPTURE defined) call
it, so the default build is unchanged.

It counts where each shadow ray of the Galerkin solver is answered (shadow
cache, restricted geometry list produced by shaft culling, or full-scene
index) and writes every list and index query, with its answer, to a binary
corpus file that later index implementations are replayed against.

Corpus format, all fields 32-bit, host byte order (little-endian on the
reference machines):

    header   char magic[8] = "RPKVQC01", int32 version = 1
    record   int32 tag, then by tag:
      1 GEOMETRY_LIST  int32 count, int32 geometryId[count]
                       The candidate list of the restricted-list queries
                       that follow, until the next GEOMETRY_LIST.
      2 QUERY          int32 path, int32 hitFlags,
                       float origin[3], float direction[3],
                       float minimumDistance, float maximumDistance,
                       int32 excludedPatchId[4] (0 = none),
                       int32 excludedGeometryId[2] (-1 = none),
                       int32 blockerPatchId (0 = no hit),
                       float blockerDistance (maximumDistance when no hit)

Paths: 0 Galerkin restricted list, 1 Galerkin full-scene index,
2 raytracer closest hit, 3 raytracer visibility, 4 stochastic radiosity
shooting. Patch identifiers are Patch::getId() (from 1), geometry
identifiers Geometry::id, both deterministic for a given scene and options.

Workspace role: owned by the scene, reached through the caller's
SceneQueryWorkspace; one per worker if ever used concurrently.
*/
class VisibilityQueryRecorder {
  public:
    enum Path {
        GALERKIN_RESTRICTED_LIST = 0,
        GALERKIN_FULL_SCENE = 1,
        RAYTRACER_CLOSEST_HIT = 2,
        RAYTRACER_VISIBILITY = 3,
        STOCHASTIC_SHOOTING = 4,
        NUMBER_OF_PATHS = 5
    };

  private:
    FILE *corpus;
    char *corpusFileName;
    long long cacheResolved;
    long long queries[NUMBER_OF_PATHS];
    long long hits[NUMBER_OF_PATHS];
    long long geometryLists;

    void writeInt(int value);
    void writeFloat(float value);

  public:
    explicit VisibilityQueryRecorder(const char *inCorpusFileName);
    ~VisibilityQueryRecorder();
    VisibilityQueryRecorder(const VisibilityQueryRecorder &) = delete;
    VisibilityQueryRecorder &operator=(const VisibilityQueryRecorder &) = delete;

    void countCacheResolved();
    void recordGeometryList(const java::ArrayList<RpkGeometry *> *geometryList);
    void recordQuery(
        Path path,
        const RpkRay *ray,
        float minimumDistance,
        float maximumDistance,
        int hitFlags,
        const RayExclusionSet &exclusions,
        const RpkRayHit *hit,
        float hitDistance);
    void printSummary() const;
};

#endif

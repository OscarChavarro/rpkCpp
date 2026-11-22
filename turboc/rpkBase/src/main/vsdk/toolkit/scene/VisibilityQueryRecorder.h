#ifndef __VISIBILITY_QUERY_RECORDER__
#define __VISIBILITY_QUERY_RECORDER__

#include <stdio.h>
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/RpkRay.h"
#include "vsdk/toolkit/environment/geometry/elements/RayExclusionSet.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHit.h"
#include "vsdk/toolkit/skin/Geometry.h"

/**
Ray accounting and ray corpus capture of the visibility queries of one run.
Only builds configured with WITH_RAY_CAPTURE (RPK_RAY_CAPTURE defined) call
it, so the default build is unchanged.

It counts where each shadow ray of the Galerkin solver is answered (shadow
cache, restricted geometry list produced by shaft culling, or full-scene
index) and writes every list and index query, with its answer, to a binary
corpus file that later index implementations are replayed against.

Corpus format, all fields 32-bit little-endian (two's complement integers,
IEEE 754 single precision floats):

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
shooting. Patch identifiers are Patch::id (from 1), geometry identifiers
Geometry::id, both deterministic for a given scene and options.

C++ port note: the C++ port writes each field with fwrite in host byte
order, which is little-endian on its reference machines; this port writes
the bytes in little-endian order explicitly, so that a corpus is the same
file on both ports whatever the size of int (16 bits on Turbo C++ 3.0).
The C++ counters are long long; here they are doubles holding whole
numbers, exact up to 2^53 and printed with the same digits, since Turbo
C++ 3.0 has no 64-bit integer (and C++98 no long long). The corpus is
written sequentially and never seeked, so a file beyond 2 GB needs only a
C library able to write it: the CMake build compiles this file with 64-bit
file offsets; on DOS a FAT16 volume caps a file at 2 GB.

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
    double cacheResolved;
    double queries[NUMBER_OF_PATHS];
    double hits[NUMBER_OF_PATHS];
    double geometryLists;

    void writeInt(long value);
    void writeFloat(float value);

    // Not copyable
    VisibilityQueryRecorder(const VisibilityQueryRecorder &);
    VisibilityQueryRecorder &operator=(const VisibilityQueryRecorder &);

  public:
    explicit VisibilityQueryRecorder(const char *inCorpusFileName);
    ~VisibilityQueryRecorder();

    void countCacheResolved();
    void recordGeometryList(const ArrayList<Geometry *> *geometryList);
    void recordQuery(
        Path path,
        const RpkRay *ray,
        float minimumDistance,
        float maximumDistance,
        int hitFlags,
        const RayExclusionSet &exclusions,
        const RayHit *hit,
        float hitDistance);
    void printSummary() const;
};

#endif

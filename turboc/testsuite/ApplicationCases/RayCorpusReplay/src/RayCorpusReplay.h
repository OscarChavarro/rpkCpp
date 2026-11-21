#ifndef __RAY_CORPUS_REPLAY__
#define __RAY_CORPUS_REPLAY__

#include <stdio.h>
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialIndex.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
#include "vsdk/toolkit/scene/VoxelGridWorkspace.h"
#include "app/SceneConsumer.h"
#include "FaceAncestry.h"
#include "ReplayAnswer.h"
#include "VisibilityQueryRecord.h"

class Patch;

/**
Plan step 4, item 4 (doc/vitralAlignmentPlan.md): replays the full-scene
visibility queries of a step 1 ray corpus (paths 1 to 4 of
VisibilityQueryRecorder) through every scene index implementation, with
RenderPark's exact face test deciding each hit:

- the legacy VoxelGrid, which must reproduce the captured answer bit for
  bit (this checks the replay itself: same scene, same arithmetic);
- VITRAL's LinearSpatialIndex, the reference (every face, in face order);
- VITRAL's BinnedSahSpatialIndex (WALD2007);
- VITRAL's GridSpatialIndex, the port of the voxel grid, built from the
  cluster hierarchy as the legacy grid is. Beyond visibility, its blocker
  and distance are compared with the legacy grid's for every query, ANY-flag
  ones included, since it follows the legacy any-hit order.

Visibility must agree for every query. For closest-hit queries the blocker
and distance must agree too; a different blocker at the same distance is
counted as a tie (a characterization, not a failure). ANY-flag queries are
answered by the indexes in ANY_HIT mode, so only their visibility is
compared. The replay is then repeated by several workers sharing the scene
and the indexes, one workspace each, and every answer must equal the
sequential one bit for bit.

Restricted-list queries (path 0) are counted and skipped: they do not reach
a scene index, and the shaft culling patch sets they name exist only during
the run that wrote them. Geometry exclusions (Galerkin full-scene path) are
mapped to the geometries of the cluster hierarchy (FaceAncestry); a query
naming one the scene does not have is counted as unsupported.

Every disagreement is written, with its ray, to the report file, for the
owner to decide (plan decision D5). The exit status is non-zero on any
capture mismatch, disagreement other than a tie, worker mismatch or
unsupported query.

C++ port note: Turbo C++ 3.0 has no threads and C++98 no std::thread, so the
workers of the second pass run one after another, each over its own
interleaved share of the chunk (query first + k * workers) with its own
workspaces, as the C++ threads do. This checks that an answer depends only
on the shared read-only scene and indexes and the worker's own workspaces,
not on a concurrent execution. The 64-bit counters of the C++ port are
doubles holding whole numbers (exact up to 2^53, printed with the same
digits), timings come from System::currentTimeMillis, and a chunk holds
fewer queries under Turbo C++ (64 KB allocations).
*/
class RayCorpusReplay : public SceneConsumer {
  public:
    enum Implementation {
        LEGACY_VOXEL_GRID = 0,
        LINEAR_REFERENCE = 1,
        BINNED_SAH = 2,
        PORTED_GRID = 3,
        NUMBER_OF_IMPLEMENTATIONS = 4
    };

  private:
    enum {
        NUMBER_OF_PATHS = 5
    };

    const char *corpusFileName;
    const char *reportFileName;
    int numberOfThreads;
    double linearStride;
    double queryLimit;

    const Scene *scene;
    ArrayList<const Patch *> patchById;
    FaceAncestry *ancestry;
    VoxelGrid *legacyGrid;
    SpatialIndex *linearIndex;
    SpatialIndex *binnedIndex;
    SpatialIndex *gridIndex;
    FILE *report;

    double queriesByPath[NUMBER_OF_PATHS];
    double replayed;
    double linearReplayed;
    double unsupported;
    double captureMismatches;
    double visibilityDisagreements[NUMBER_OF_IMPLEMENTATIONS];
    double closestDisagreements[NUMBER_OF_IMPLEMENTATIONS];
    double closestTies[NUMBER_OF_IMPLEMENTATIONS];
    double binnedVersusLinear;
    double gridBlockerDifferences;
    double threadMismatches;
    double reportedLines;
    double primitiveTests[NUMBER_OF_IMPLEMENTATIONS];
    double nodeVisits[NUMBER_OF_IMPLEMENTATIONS];
    double seconds[NUMBER_OF_IMPLEMENTATIONS];

    void buildIndexes();
    bool isSupported(const VisibilityQueryRecord &record) const;
    bool usesLinear(double queryNumber) const;
    ReplayAnswer answerLegacy(const VisibilityQueryRecord &record, VoxelGridWorkspace &workspace) const;
    ReplayAnswer answerIndex(
        const SpatialIndex &index,
        const VisibilityQueryRecord &record,
        SpatialIndexWorkspace &workspace) const;
    void answerRange(
        const VisibilityQueryRecord *records,
        int numberOfRecords,
        double firstQueryNumber,
        int first,
        int step,
        ReplayAnswer *answers) const;
    void compare(
        const VisibilityQueryRecord &record,
        double queryNumber,
        const ReplayAnswer *answers);
    bool belowExcludedGeometry(const VisibilityQueryRecord &record, const ReplayAnswer &answer) const;
    void reportLine(
        const char *kind,
        const VisibilityQueryRecord &record,
        Implementation implementation,
        const ReplayAnswer &expected,
        const ReplayAnswer &found);
    void replayChunk(const VisibilityQueryRecord *records, int numberOfRecords, double firstQueryNumber);
    void printSummary() const;
    bool passed() const;

    // Not copyable
    RayCorpusReplay(const RayCorpusReplay &);
    RayCorpusReplay &operator=(const RayCorpusReplay &);

  public:
    RayCorpusReplay(
        const char *inCorpusFileName,
        const char *inReportFileName,
        int inNumberOfThreads,
        double inLinearStride,
        double inQueryLimit);
    virtual ~RayCorpusReplay();

    virtual int consume(const Scene &inScene);
};

#endif

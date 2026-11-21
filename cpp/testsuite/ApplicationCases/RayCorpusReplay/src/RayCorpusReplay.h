#ifndef RAY_CORPUS_REPLAY__
#define RAY_CORPUS_REPLAY__

#include <cstdio>
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialIndex.h"
#include "vsdk/toolkit/scene/VoxelGridWorkspace.h"
#include "SceneConsumer.h"
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
- VITRAL's BinnedSahSpatialIndex (WALD2007).

Visibility must agree for every query. For closest-hit queries the blocker
and distance must agree too; a different blocker at the same distance is
counted as a tie (a characterization, not a failure). ANY-flag queries are
answered by the indexes in ANY_HIT mode, so only their visibility is
compared. The replay is then repeated from several threads sharing the scene
and the indexes, one workspace each, and every answer must equal the
sequential one bit for bit.

Restricted-list queries (path 0) are counted and skipped: they do not reach
a scene index, and the shaft culling patch sets they name exist only during
the run that wrote them. Geometry exclusions (Galerkin full-scene path) map to
faces and are counted as unsupported (only the Galerkin full-scene path
issues them; no stored corpus has one).

Every disagreement is written, with its ray, to the report file, for the
owner to decide (plan decision D5). The exit status is non-zero on any
capture mismatch, disagreement other than a tie, thread mismatch or
unsupported query.
*/
class RayCorpusReplay final : public SceneConsumer {
  public:
    enum Implementation {
        LEGACY_VOXEL_GRID = 0,
        LINEAR_REFERENCE = 1,
        BINNED_SAH = 2,
        NUMBER_OF_IMPLEMENTATIONS = 3
    };

  private:
    static const int NUMBER_OF_PATHS = 5;

    const char *corpusFileName;
    const char *reportFileName;
    int numberOfThreads;
    long long linearStride;
    long long queryLimit;

    const Scene *scene;
    java::ArrayList<const Patch *> patchById;
    FaceAncestry *ancestry;
    SpatialIndex *linearIndex;
    SpatialIndex *binnedIndex;
    FILE *report;

    long long queriesByPath[NUMBER_OF_PATHS];
    long long replayed;
    long long linearReplayed;
    long long unsupported;
    long long captureMismatches;
    long long visibilityDisagreements[NUMBER_OF_IMPLEMENTATIONS];
    long long closestDisagreements[NUMBER_OF_IMPLEMENTATIONS];
    long long closestTies[NUMBER_OF_IMPLEMENTATIONS];
    long long binnedVersusLinear;
    long long threadMismatches;
    long long reportedLines;
    long long primitiveTests[NUMBER_OF_IMPLEMENTATIONS];
    long long nodeVisits[NUMBER_OF_IMPLEMENTATIONS];
    double seconds[NUMBER_OF_IMPLEMENTATIONS];

    void buildIndexes();
    bool isSupported(const VisibilityQueryRecord &record) const;
    bool usesLinear(long long queryNumber) const;
    ReplayAnswer answerLegacy(const VisibilityQueryRecord &record, VoxelGridWorkspace &workspace) const;
    ReplayAnswer answerIndex(
        const SpatialIndex &index,
        const VisibilityQueryRecord &record,
        SpatialIndexWorkspace &workspace) const;
    void answerRange(
        const VisibilityQueryRecord *records,
        int numberOfRecords,
        long long firstQueryNumber,
        int first,
        int step,
        ReplayAnswer *answers) const;
    void compare(
        const VisibilityQueryRecord &record,
        long long queryNumber,
        const ReplayAnswer *answers);
    bool belowExcludedGeometry(const VisibilityQueryRecord &record, const ReplayAnswer &answer) const;
    void reportLine(
        const char *kind,
        const VisibilityQueryRecord &record,
        Implementation implementation,
        const ReplayAnswer &expected,
        const ReplayAnswer &found);
    void replayChunk(const VisibilityQueryRecord *records, int numberOfRecords, long long firstQueryNumber);
    void printSummary() const;
    bool passed() const;

  public:
    RayCorpusReplay(
        const char *inCorpusFileName,
        const char *inReportFileName,
        int inNumberOfThreads,
        long long inLinearStride,
        long long inQueryLimit);
    ~RayCorpusReplay() final;
    RayCorpusReplay(const RayCorpusReplay &) = delete;
    RayCorpusReplay &operator=(const RayCorpusReplay &) = delete;

    int consume(const Scene &inScene) final;
};

#endif

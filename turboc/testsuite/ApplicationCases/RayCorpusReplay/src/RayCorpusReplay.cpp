#include <math.h>
#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/GridSpatialIndex.h"
#include "vsdk/toolkit/scene/SceneFaceRayTest.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
#include "RayCorpusReader.h"
#include "RayCorpusReplay.h"

// Queries read and answered at a time; the worker pass repeats each chunk
#ifdef __TURBOC__
static const int CHUNK_SIZE = 512;
#else
static const int CHUNK_SIZE = 1 << 20;
#endif

// Disagreement lines written to the report, per run
static const double MAXIMUM_REPORTED_LINES = 100000.0;

static const char *IMPLEMENTATION_NAMES[RayCorpusReplay::NUMBER_OF_IMPLEMENTATIONS] = {
    "legacy voxel grid",
    "linear reference",
    "binned SAH (WALD2007)",
    "ported voxel grid"
};

static double
nowSeconds() {
    return ((double)(System::currentTimeMillis())) / 1000.0;
}

RayCorpusReplay::RayCorpusReplay(
    const char *inCorpusFileName,
    const char *inReportFileName,
    const int inNumberOfThreads,
    const double inLinearStride,
    const double inQueryLimit):
    corpusFileName(inCorpusFileName),
    reportFileName(inReportFileName),
    numberOfThreads(inNumberOfThreads > 0 ? inNumberOfThreads : 1),
    linearStride(inLinearStride),
    queryLimit(inQueryLimit),
    scene(NULL),
    patchById(),
    ancestry(NULL),
    legacyGrid(NULL),
    linearIndex(NULL),
    binnedIndex(NULL),
    gridIndex(NULL),
    report(NULL),
    replayed(0.0),
    linearReplayed(0.0),
    unsupported(0.0),
    captureMismatches(0.0),
    binnedVersusLinear(0.0),
    gridBlockerDifferences(0.0),
    threadMismatches(0.0),
    reportedLines(0.0)
{
    int i;
    for ( i = 0; i < NUMBER_OF_PATHS; i++ ) {
        queriesByPath[i] = 0.0;
    }
    for ( i = 0; i < NUMBER_OF_IMPLEMENTATIONS; i++ ) {
        visibilityDisagreements[i] = 0.0;
        closestDisagreements[i] = 0.0;
        closestTies[i] = 0.0;
        primitiveTests[i] = 0.0;
        nodeVisits[i] = 0.0;
        seconds[i] = 0.0;
    }
}

RayCorpusReplay::~RayCorpusReplay() {
    delete legacyGrid;
    legacyGrid = NULL;
    delete linearIndex;
    linearIndex = NULL;
    delete binnedIndex;
    binnedIndex = NULL;
    delete gridIndex;
    gridIndex = NULL;
    delete ancestry;
    ancestry = NULL;
    if ( report != NULL ) {
        fclose(report);
        report = NULL;
    }
}

/**
The legacy grid as the scene builder built it before the scene query, and
the VITRAL indexes as the scene query builds them (SceneQuery::buildIndex):
primitive i is face i of the scene's face table.
*/
void
RayCorpusReplay::buildIndexes() {
    const int numberOfFaces = scene->getNumberOfFaces();
    ancestry = new FaceAncestry(*scene);
    legacyGrid = new VoxelGrid(scene->clusteredRootGeometry);
    linearIndex = SceneQuery::buildIndex(*scene, BRUTE_FORCE_ACCELERATION);
    binnedIndex = SceneQuery::buildIndex(*scene, WALD2007_ACCELERATION);
    gridIndex = SceneQuery::buildIndex(*scene, VOXEL_ACCELERATION);

    long maximumId = 0;
    int i;
    for ( i = 0; i < numberOfFaces; i++ ) {
        const long id = ((long)(scene->getFace(i)->id));
        maximumId = id > maximumId ? id : maximumId;
    }
    for ( long id = 0; id <= maximumId; id++ ) {
        patchById.add(NULL);
    }
    for ( i = 0; i < numberOfFaces; i++ ) {
        const Patch *patch = scene->getFace(i);
        patchById.set(((long)(patch->id)), patch);
    }
}

bool
RayCorpusReplay::isSupported(const VisibilityQueryRecord &record) const {
    int i;
    for ( i = 0; i < VisibilityQueryRecord::NUMBER_OF_EXCLUDED_GEOMETRIES; i++ ) {
        const long id = record.getExcludedGeometryId(i);
        if ( id != -1 && ancestry->getGeometry(id) == NULL ) {
            return false;
        }
    }
    for ( i = 0; i < VisibilityQueryRecord::NUMBER_OF_EXCLUDED_PATCHES; i++ ) {
        const long id = record.getExcludedPatchId(i);
        if ( id < 0 || id >= patchById.size() || (id != 0 && patchById.get(id) == NULL) ) {
            return false;
        }
    }
    return true;
}

bool
RayCorpusReplay::usesLinear(const double queryNumber) const {
    return linearStride > 0.0 && fmod(queryNumber, linearStride) == 0.0;
}

static RpkRay
recordRay(const VisibilityQueryRecord &record) {
    RpkRay ray;
    ray.position = Vector3D(record.getOrigin(0), record.getOrigin(1), record.getOrigin(2));
    ray.direction = Vector3D(record.getDirection(0), record.getDirection(1), record.getDirection(2));
    return ray;
}

static RayExclusionSet
recordExclusions(
    const VisibilityQueryRecord &record,
    const ArrayList<const Patch *> &patchById,
    const FaceAncestry &ancestry)
{
    const Patch *excluded[VisibilityQueryRecord::NUMBER_OF_EXCLUDED_PATCHES];
    for ( int i = 0; i < VisibilityQueryRecord::NUMBER_OF_EXCLUDED_PATCHES; i++ ) {
        const long id = record.getExcludedPatchId(i);
        excluded[i] = id != 0 ? patchById.get(id) : NULL;
    }
    const long geometry1 = record.getExcludedGeometryId(0);
    const long geometry2 = record.getExcludedGeometryId(1);
    return RayExclusionSet(
        excluded[0], excluded[1], excluded[2], excluded[3],
        geometry1 != -1 ? ancestry.getGeometry(geometry1) : NULL,
        geometry2 != -1 ? ancestry.getGeometry(geometry2) : NULL);
}

ReplayAnswer
RayCorpusReplay::answerLegacy(const VisibilityQueryRecord &record, VoxelGridWorkspace &workspace) const {
    RpkRay ray = recordRay(record);
    const RayExclusionSet exclusions = recordExclusions(record, patchById, *ancestry);
    RayHit hitStore;
    float distance = record.getMaximumDistance();
    const RayHit *hit = legacyGrid->gridIntersect(
        &ray, record.getMinimumDistance(), &distance, ((int)(record.getHitFlags())), &hitStore, exclusions, &workspace);
    if ( hit == NULL ) {
        return ReplayAnswer(0, record.getMaximumDistance());
    }
    return ReplayAnswer(((long)(hit->getPatch()->id)), distance);
}

ReplayAnswer
RayCorpusReplay::answerIndex(
    const SpatialIndex &index,
    const VisibilityQueryRecord &record,
    SpatialIndexWorkspace &workspace) const
{
    RpkRay ray = recordRay(record);
    const RayExclusionSet exclusions = recordExclusions(record, patchById, *ancestry);
    RayHit hitStore;
    SceneFaceRayTest test(*scene, &ray, ((int)(record.getHitFlags())), exclusions, &hitStore);
    const Ray indexRay = Ray::fromAffine(
        Vector3Dd(ray.position.x, ray.position.y, ray.position.z),
        Vector3Dd(ray.direction.x, ray.direction.y, ray.direction.z));
    const SpatialQueryMode mode = (record.getHitFlags() & ANY) != 0 ? ANY_HIT : CLOSEST_HIT;
    const SpatialQueryResult result = index.query(
        indexRay, record.getMinimumDistance(), record.getMaximumDistance(), mode, test, workspace);
    if ( !result.hasHit() ) {
        return ReplayAnswer(0, record.getMaximumDistance());
    }
    return ReplayAnswer(
        ((long)(scene->getFace(result.getPrimitive())->id)),
        ((float)(result.getParameter())));
}

/**
Answers records first, first + step, ... with every implementation (the
linear reference only on its sampled queries) into
answers[NUMBER_OF_IMPLEMENTATIONS * record]. Used by the sequential pass
(step 1) and by each worker of the worker pass, with its own workspaces.
*/
void
RayCorpusReplay::answerRange(
    const VisibilityQueryRecord *records,
    const int numberOfRecords,
    const double firstQueryNumber,
    const int first,
    const int step,
    ReplayAnswer *answers) const
{
    VoxelGridWorkspace gridWorkspace(legacyGrid->getNumberOfItems());
    SpatialIndexWorkspace linearWorkspace(linearIndex->getWorkspaceSize());
    SpatialIndexWorkspace binnedWorkspace(binnedIndex->getWorkspaceSize());
    SpatialIndexWorkspace portedWorkspace(gridIndex->getWorkspaceSize());
    for ( int i = first; i < numberOfRecords; i += step ) {
        ReplayAnswer *answer = &answers[NUMBER_OF_IMPLEMENTATIONS * i];
        answer[LEGACY_VOXEL_GRID] = answerLegacy(records[i], gridWorkspace);
        answer[BINNED_SAH] = answerIndex(*binnedIndex, records[i], binnedWorkspace);
        answer[PORTED_GRID] = answerIndex(*gridIndex, records[i], portedWorkspace);
        if ( usesLinear(firstQueryNumber + i) ) {
            answer[LINEAR_REFERENCE] = answerIndex(*linearIndex, records[i], linearWorkspace);
        }
    }
}

bool
RayCorpusReplay::belowExcludedGeometry(const VisibilityQueryRecord &record, const ReplayAnswer &answer) const {
    if ( !answer.hasHit() || answer.getBlockerPatchId() >= patchById.size() ) {
        return false;
    }
    const Patch *blocker = patchById.get(answer.getBlockerPatchId());
    return blocker != NULL
        && ancestry->excludesFace(blocker->getFaceIndex(), recordExclusions(record, patchById, *ancestry));
}

void
RayCorpusReplay::reportLine(
    const char *kind,
    const VisibilityQueryRecord &record,
    const Implementation implementation,
    const ReplayAnswer &expected,
    const ReplayAnswer &found)
{
    if ( report == NULL || reportedLines >= MAXIMUM_REPORTED_LINES ) {
        return;
    }
    reportedLines += 1.0;
    fprintf(report,
        "%s|%s|path %ld|flags 0x%lx|origin %.9g %.9g %.9g|direction %.9g %.9g %.9g|interval %.9g %.9g"
        "|excluded %ld %ld %ld %ld|excluded geometries %ld %ld|expected %ld %.9g|found %ld %.9g"
        "|expected blocker below an excluded geometry %s|found blocker below an excluded geometry %s\n",
        kind, IMPLEMENTATION_NAMES[implementation], record.getPath(), ((unsigned long)(record.getHitFlags())),
        record.getOrigin(0), record.getOrigin(1), record.getOrigin(2),
        record.getDirection(0), record.getDirection(1), record.getDirection(2),
        record.getMinimumDistance(), record.getMaximumDistance(),
        record.getExcludedPatchId(0), record.getExcludedPatchId(1),
        record.getExcludedPatchId(2), record.getExcludedPatchId(3),
        record.getExcludedGeometryId(0), record.getExcludedGeometryId(1),
        expected.getBlockerPatchId(), expected.getDistance(),
        found.getBlockerPatchId(), found.getDistance(),
        belowExcludedGeometry(record, expected) ? "yes" : "no",
        belowExcludedGeometry(record, found) ? "yes" : "no");
}

/**
Compares the sequential answers of one query: the legacy grid with the
capture, then each index with the legacy grid, and the binned index with the
linear reference.
*/
void
RayCorpusReplay::compare(
    const VisibilityQueryRecord &record,
    const double queryNumber,
    const ReplayAnswer *answers)
{
    const ReplayAnswer captured(record.getBlockerPatchId(), record.getBlockerDistance());
    const ReplayAnswer &legacy = answers[LEGACY_VOXEL_GRID];
    if ( !legacy.identical(captured) ) {
        captureMismatches += 1.0;
        reportLine("capture mismatch", record, LEGACY_VOXEL_GRID, captured, legacy);
    }
    const bool closest = (record.getHitFlags() & ANY) == 0;
    const bool linear = usesLinear(queryNumber);
    for ( int implementation = LINEAR_REFERENCE; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++ ) {
        if ( implementation == LINEAR_REFERENCE && !linear ) {
            continue;
        }
        const ReplayAnswer &other = answers[implementation];
        const Implementation which = ((Implementation)(implementation));
        if ( other.hasHit() != legacy.hasHit() ) {
            visibilityDisagreements[implementation] += 1.0;
            reportLine("visibility", record, which, legacy, other);
        } else if ( closest && !other.identical(legacy) ) {
            if ( other.hasHit() && other.sameDistance(legacy) ) {
                closestTies[implementation] += 1.0;
                reportLine("tie", record, which, legacy, other);
            } else {
                closestDisagreements[implementation] += 1.0;
                reportLine("closest hit", record, which, legacy, other);
            }
        }
    }
    // The ported grid follows the legacy traversal order, any-hit included
    if ( !answers[PORTED_GRID].identical(legacy) ) {
        gridBlockerDifferences += 1.0;
        if ( answers[PORTED_GRID].hasHit() == legacy.hasHit() && (closest ? answers[PORTED_GRID].sameDistance(legacy) : true) ) {
            reportLine("ported grid blocker", record, PORTED_GRID, legacy, answers[PORTED_GRID]);
        }
    }
    if ( linear ) {
        const ReplayAnswer &reference = answers[LINEAR_REFERENCE];
        const ReplayAnswer &binned = answers[BINNED_SAH];
        if ( reference.hasHit() != binned.hasHit()
             || (closest && !binned.identical(reference) && !binned.sameDistance(reference)) ) {
            binnedVersusLinear += 1.0;
            reportLine("binned versus linear", record, BINNED_SAH, reference, binned);
        }
    }
}

void
RayCorpusReplay::replayChunk(
    const VisibilityQueryRecord *records,
    const int numberOfRecords,
    const double firstQueryNumber)
{
    ReplayAnswer *sequential = new ReplayAnswer[NUMBER_OF_IMPLEMENTATIONS * numberOfRecords];

    // Sequential pass, one implementation at a time for the timings
    {
        VoxelGridWorkspace gridWorkspace(legacyGrid->getNumberOfItems());
        double start = nowSeconds();
        int i;
        for ( i = 0; i < numberOfRecords; i++ ) {
            sequential[NUMBER_OF_IMPLEMENTATIONS * i + LEGACY_VOXEL_GRID] = answerLegacy(records[i], gridWorkspace);
        }
        seconds[LEGACY_VOXEL_GRID] += nowSeconds() - start;

        SpatialIndexWorkspace binnedWorkspace(binnedIndex->getWorkspaceSize());
        start = nowSeconds();
        for ( i = 0; i < numberOfRecords; i++ ) {
            sequential[NUMBER_OF_IMPLEMENTATIONS * i + BINNED_SAH] = answerIndex(*binnedIndex, records[i], binnedWorkspace);
        }
        seconds[BINNED_SAH] += nowSeconds() - start;
        primitiveTests[BINNED_SAH] += ((double)(binnedWorkspace.getPrimitiveTests()));
        nodeVisits[BINNED_SAH] += ((double)(binnedWorkspace.getNodeVisits()));

        SpatialIndexWorkspace portedWorkspace(gridIndex->getWorkspaceSize());
        start = nowSeconds();
        for ( i = 0; i < numberOfRecords; i++ ) {
            sequential[NUMBER_OF_IMPLEMENTATIONS * i + PORTED_GRID] = answerIndex(*gridIndex, records[i], portedWorkspace);
        }
        seconds[PORTED_GRID] += nowSeconds() - start;
        primitiveTests[PORTED_GRID] += ((double)(portedWorkspace.getPrimitiveTests()));
        nodeVisits[PORTED_GRID] += ((double)(portedWorkspace.getNodeVisits()));

        SpatialIndexWorkspace linearWorkspace(linearIndex->getWorkspaceSize());
        start = nowSeconds();
        for ( i = 0; i < numberOfRecords; i++ ) {
            if ( usesLinear(firstQueryNumber + i) ) {
                sequential[NUMBER_OF_IMPLEMENTATIONS * i + LINEAR_REFERENCE] = answerIndex(*linearIndex, records[i], linearWorkspace);
                linearReplayed += 1.0;
            }
        }
        seconds[LINEAR_REFERENCE] += nowSeconds() - start;
        primitiveTests[LINEAR_REFERENCE] += ((double)(linearWorkspace.getPrimitiveTests()));
    }

    for ( int i = 0; i < numberOfRecords; i++ ) {
        compare(records[i], firstQueryNumber + i, &sequential[NUMBER_OF_IMPLEMENTATIONS * i]);
    }

    // Worker pass: the workers share the scene and the indexes, each with its own workspaces
    if ( numberOfThreads > 1 ) {
        ReplayAnswer *threaded = new ReplayAnswer[NUMBER_OF_IMPLEMENTATIONS * numberOfRecords];
        for ( int t = 0; t < numberOfThreads; t++ ) {
            answerRange(records, numberOfRecords, firstQueryNumber, t, numberOfThreads, threaded);
        }
        for ( int i = 0; i < numberOfRecords; i++ ) {
            for ( int implementation = 0; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++ ) {
                if ( implementation == LINEAR_REFERENCE && !usesLinear(firstQueryNumber + i) ) {
                    continue;
                }
                const ReplayAnswer &expected = sequential[NUMBER_OF_IMPLEMENTATIONS * i + implementation];
                const ReplayAnswer &found = threaded[NUMBER_OF_IMPLEMENTATIONS * i + implementation];
                if ( !found.identical(expected) ) {
                    threadMismatches += 1.0;
                    reportLine("thread mismatch", records[i], ((Implementation)(implementation)), expected, found);
                }
            }
        }
        delete[] threaded;
    }
    delete[] sequential;
}

void
RayCorpusReplay::printSummary() const {
    fprintf(stdout, "Ray corpus replay: %s\n", corpusFileName);
    fprintf(stdout, "  faces %d, voxel grid items %d, threads %d, linear reference on 1 query in %.0f\n",
        scene->getFaceBounds()->getNumberOfFaces(), legacyGrid->getNumberOfItems(), numberOfThreads, linearStride);
    for ( int path = 0; path < NUMBER_OF_PATHS; path++ ) {
        fprintf(stdout, "  queries of path %d: %.0f%s\n", path, queriesByPath[path], path == 0 ? " (restricted list, not replayed)" : "");
    }
    fprintf(stdout, "  replayed %.0f (linear reference %.0f), unsupported %.0f\n", replayed, linearReplayed, unsupported);
    fprintf(stdout, "  legacy voxel grid against the capture: %.0f mismatches\n", captureMismatches);
    int implementation;
    for ( implementation = LINEAR_REFERENCE; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++ ) {
        fprintf(stdout, "  %s against the legacy grid: visibility %.0f, closest hit %.0f, ties %.0f\n",
            IMPLEMENTATION_NAMES[implementation], visibilityDisagreements[implementation],
            closestDisagreements[implementation], closestTies[implementation]);
    }
    fprintf(stdout, "  binned SAH against the linear reference: %.0f disagreements\n", binnedVersusLinear);
    fprintf(stdout, "  ported voxel grid: %d items, %d grids; blocker or distance other than the legacy grid's (any query): %.0f\n",
        ((const GridSpatialIndex *)(gridIndex))->getNumberOfItems(),
        ((const GridSpatialIndex *)(gridIndex))->getNumberOfGrids(), gridBlockerDifferences);
    fprintf(stdout, "  threaded against sequential: %.0f mismatches\n", threadMismatches);
    for ( implementation = 0; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++ ) {
        fprintf(stdout, "  time %s: %.2f s, primitive tests %.0f, node visits %.0f\n",
            IMPLEMENTATION_NAMES[implementation], seconds[implementation],
            primitiveTests[implementation], nodeVisits[implementation]);
    }
    fprintf(stdout, "  %s\n", passed() ? "PASSED" : "FAILED");
}

bool
RayCorpusReplay::passed() const {
    bool ok = replayed > 0.0 && unsupported == 0.0 && captureMismatches == 0.0 && binnedVersusLinear == 0.0 && threadMismatches == 0.0;
    for ( int implementation = LINEAR_REFERENCE; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++ ) {
        ok = ok && visibilityDisagreements[implementation] == 0.0 && closestDisagreements[implementation] == 0.0;
    }
    return ok;
}

int
RayCorpusReplay::consume(const Scene &inScene) {
    scene = &inScene;
    RayCorpusReader reader(corpusFileName);
    if ( !reader.isValid() ) {
        fprintf(stderr, "Ray corpus replay: %s is not a readable ray corpus\n", corpusFileName);
        return 2;
    }
    if ( reportFileName != NULL && reportFileName[0] != '\0' ) {
        report = fopen(reportFileName, "w");
    }
    buildIndexes();

    VisibilityQueryRecord *records = new VisibilityQueryRecord[CHUNK_SIZE];
    int count = 0;
    VisibilityQueryRecord record;
    while ( (queryLimit <= 0.0 || replayed + count < queryLimit) && reader.next(&record) ) {
        const long path = record.getPath();
        if ( path < 0 || path >= NUMBER_OF_PATHS ) {
            unsupported += 1.0;
            continue;
        }
        queriesByPath[path] += 1.0;
        if ( path == 0 ) {
            continue;
        }
        if ( !isSupported(record) ) {
            unsupported += 1.0;
            continue;
        }
        records[count++] = record;
        if ( count == CHUNK_SIZE ) {
            replayChunk(records, count, replayed);
            replayed += count;
            count = 0;
            fprintf(stderr, "Ray corpus replay: %.0f queries\n", replayed);
        }
    }
    if ( count > 0 ) {
        replayChunk(records, count, replayed);
        replayed += count;
    }
    delete[] records;
    if ( !reader.isValid() ) {
        fprintf(stderr, "Ray corpus replay: %s is malformed after %.0f queries\n", corpusFileName, replayed);
        return 2;
    }
    printSummary();
    return passed() ? 0 : 1;
}

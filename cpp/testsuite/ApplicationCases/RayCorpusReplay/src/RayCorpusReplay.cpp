#include <chrono>
#include <thread>
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/BinnedSahSpatialIndex.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/LinearSpatialIndex.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
#include "RayCorpusReader.h"
#include "RayCorpusReplay.h"
#include "RpkFaceRayTest.h"

// Queries read and answered at a time; the threaded pass repeats each chunk
static const int CHUNK_SIZE = 1 << 20;

// Disagreement lines written to the report, per run
static const long long MAXIMUM_REPORTED_LINES = 100000;

static const char *IMPLEMENTATION_NAMES[RayCorpusReplay::NUMBER_OF_IMPLEMENTATIONS] = {
    "legacy voxel grid",
    "linear reference",
    "binned SAH (WALD2007)"
};

static double
secondsSince(const std::chrono::steady_clock::time_point &start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

RayCorpusReplay::RayCorpusReplay(
    const char *inCorpusFileName,
    const char *inReportFileName,
    const int inNumberOfThreads,
    const long long inLinearStride,
    const long long inQueryLimit):
    corpusFileName(inCorpusFileName),
    reportFileName(inReportFileName),
    numberOfThreads(inNumberOfThreads > 0 ? inNumberOfThreads : 1),
    linearStride(inLinearStride),
    queryLimit(inQueryLimit),
    scene(nullptr),
    patchById(),
    ancestry(nullptr),
    linearIndex(nullptr),
    binnedIndex(nullptr),
    report(nullptr),
    queriesByPath(),
    replayed(0),
    linearReplayed(0),
    unsupported(0),
    captureMismatches(0),
    visibilityDisagreements(),
    closestDisagreements(),
    closestTies(),
    binnedVersusLinear(0),
    threadMismatches(0),
    reportedLines(0),
    primitiveTests(),
    nodeVisits(),
    seconds()
{
    for ( int i = 0; i < NUMBER_OF_PATHS; i++ ) {
        queriesByPath[i] = 0;
    }
    for ( int i = 0; i < NUMBER_OF_IMPLEMENTATIONS; i++ ) {
        visibilityDisagreements[i] = 0;
        closestDisagreements[i] = 0;
        closestTies[i] = 0;
        primitiveTests[i] = 0;
        nodeVisits[i] = 0;
        seconds[i] = 0.0;
    }
}

RayCorpusReplay::~RayCorpusReplay() {
    delete linearIndex;
    linearIndex = nullptr;
    delete binnedIndex;
    binnedIndex = nullptr;
    delete ancestry;
    ancestry = nullptr;
    if ( report != nullptr ) {
        fclose(report);
        report = nullptr;
    }
}

/**
Primitive i is face i of the scene's face table. Each box is the face's
FaceBoundsTable box, widened to double and padded by the face's plane
tolerance plus a relative margin, so that it contains every point the exact
face test may accept.
*/
void
RayCorpusReplay::buildIndexes() {
    const FaceBoundsTable *faceBounds = scene->getFaceBounds();
    const int numberOfFaces = faceBounds->getNumberOfFaces();
    java::ArrayList<double> boxes(6L * numberOfFaces);
    for ( int i = 0; i < numberOfFaces; i++ ) {
        const RpkAxisAlignedBoundingBox &box = faceBounds->get(i);
        const double extent = box.maxExtent();
        const double low[3] = {box.minX(), box.minY(), box.minZ()};
        const double high[3] = {box.maxX(), box.maxY(), box.maxZ()};
        const double tolerance = scene->getFace(i)->getTolerance();
        for ( int axis = 0; axis < 3; axis++ ) {
            const double magnitude = java::Math::max(java::Math::abs(low[axis]), java::Math::abs(high[axis]));
            const double padding = tolerance + 1e-4 * extent + 1e-5 * magnitude;
            boxes.add(low[axis] - padding);
        }
        for ( int axis = 0; axis < 3; axis++ ) {
            const double magnitude = java::Math::max(java::Math::abs(low[axis]), java::Math::abs(high[axis]));
            const double padding = tolerance + 1e-4 * extent + 1e-5 * magnitude;
            boxes.add(high[axis] + padding);
        }
    }
    ancestry = new FaceAncestry(*scene);
    linearIndex = new LinearSpatialIndex(numberOfFaces);
    binnedIndex = new BinnedSahSpatialIndex(boxes, java::ArrayList<int>());

    int maximumId = 0;
    for ( int i = 0; i < numberOfFaces; i++ ) {
        const int id = static_cast<int>(scene->getFace(i)->getId());
        maximumId = id > maximumId ? id : maximumId;
    }
    for ( int id = 0; id <= maximumId; id++ ) {
        patchById.add(nullptr);
    }
    for ( int i = 0; i < numberOfFaces; i++ ) {
        const Patch *patch = scene->getFace(i);
        patchById.set(static_cast<long>(patch->getId()), patch);
    }
}

bool
RayCorpusReplay::isSupported(const VisibilityQueryRecord &record) const {
    for ( int i = 0; i < VisibilityQueryRecord::NUMBER_OF_EXCLUDED_GEOMETRIES; i++ ) {
        const int id = record.getExcludedGeometryId(i);
        if ( id != -1 && ancestry->getGeometry(id) == nullptr ) {
            return false;
        }
    }
    for ( int i = 0; i < VisibilityQueryRecord::NUMBER_OF_EXCLUDED_PATCHES; i++ ) {
        const int id = record.getExcludedPatchId(i);
        if ( id < 0 || id >= patchById.size() || (id != 0 && patchById.get(id) == nullptr) ) {
            return false;
        }
    }
    return true;
}

bool
RayCorpusReplay::usesLinear(const long long queryNumber) const {
    return linearStride > 0 && queryNumber % linearStride == 0;
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
    const java::ArrayList<const Patch *> &patchById,
    const FaceAncestry &ancestry)
{
    const Patch *excluded[VisibilityQueryRecord::NUMBER_OF_EXCLUDED_PATCHES];
    for ( int i = 0; i < VisibilityQueryRecord::NUMBER_OF_EXCLUDED_PATCHES; i++ ) {
        const int id = record.getExcludedPatchId(i);
        excluded[i] = id != 0 ? patchById.get(id) : nullptr;
    }
    const int geometry1 = record.getExcludedGeometryId(0);
    const int geometry2 = record.getExcludedGeometryId(1);
    return RayExclusionSet(
        excluded[0], excluded[1], excluded[2], excluded[3],
        geometry1 != -1 ? ancestry.getGeometry(geometry1) : nullptr,
        geometry2 != -1 ? ancestry.getGeometry(geometry2) : nullptr);
}

ReplayAnswer
RayCorpusReplay::answerLegacy(const VisibilityQueryRecord &record, VoxelGridWorkspace &workspace) const {
    RpkRay ray = recordRay(record);
    const RayExclusionSet exclusions = recordExclusions(record, patchById, *ancestry);
    RpkRayHit hitStore;
    float distance = record.getMaximumDistance();
    const RpkRayHit *hit = scene->voxelGrid->gridIntersect(
        &ray, record.getMinimumDistance(), &distance, record.getHitFlags(), &hitStore, exclusions, &workspace);
    if ( hit == nullptr ) {
        return ReplayAnswer(0, record.getMaximumDistance());
    }
    return ReplayAnswer(static_cast<int>(hit->getPatch()->getId()), distance);
}

ReplayAnswer
RayCorpusReplay::answerIndex(
    const SpatialIndex &index,
    const VisibilityQueryRecord &record,
    SpatialIndexWorkspace &workspace) const
{
    RpkRay ray = recordRay(record);
    const RayExclusionSet exclusions = recordExclusions(record, patchById, *ancestry);
    RpkFaceRayTest test(*scene, *ancestry, &ray, record.getHitFlags(), exclusions);
    const Ray indexRay = Ray::fromAffine(
        Vector3Dd(ray.position.x, ray.position.y, ray.position.z),
        Vector3Dd(ray.direction.x, ray.direction.y, ray.direction.z));
    const SpatialQueryMode mode = (record.getHitFlags() & RayHitFlag::ANY) != 0
        ? SpatialQueryMode::ANY_HIT : SpatialQueryMode::CLOSEST_HIT;
    const SpatialQueryResult result = index.query(
        indexRay, record.getMinimumDistance(), record.getMaximumDistance(), mode, test, workspace);
    if ( !result.hasHit() ) {
        return ReplayAnswer(0, record.getMaximumDistance());
    }
    return ReplayAnswer(
        static_cast<int>(scene->getFace(result.getPrimitive())->getId()),
        static_cast<float>(result.getParameter()));
}

/**
Answers records first, first + step, ... with every implementation (the
linear reference only on its sampled queries) into
answers[NUMBER_OF_IMPLEMENTATIONS * record]. Used by the sequential pass
(step 1) and by each worker of the threaded pass, with its own workspaces.
*/
void
RayCorpusReplay::answerRange(
    const VisibilityQueryRecord *records,
    const int numberOfRecords,
    const long long firstQueryNumber,
    const int first,
    const int step,
    ReplayAnswer *answers) const
{
    VoxelGridWorkspace gridWorkspace(scene->voxelGrid->getNumberOfItems());
    SpatialIndexWorkspace linearWorkspace(linearIndex->getWorkspaceSize());
    SpatialIndexWorkspace binnedWorkspace(binnedIndex->getWorkspaceSize());
    for ( int i = first; i < numberOfRecords; i += step ) {
        ReplayAnswer *answer = &answers[NUMBER_OF_IMPLEMENTATIONS * i];
        answer[LEGACY_VOXEL_GRID] = answerLegacy(records[i], gridWorkspace);
        answer[BINNED_SAH] = answerIndex(*binnedIndex, records[i], binnedWorkspace);
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
    return blocker != nullptr
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
    if ( report == nullptr || reportedLines >= MAXIMUM_REPORTED_LINES ) {
        return;
    }
    reportedLines++;
    fprintf(report,
        "%s|%s|path %d|flags 0x%x|origin %.9g %.9g %.9g|direction %.9g %.9g %.9g|interval %.9g %.9g"
        "|excluded %d %d %d %d|excluded geometries %d %d|expected %d %.9g|found %d %.9g"
        "|expected blocker below an excluded geometry %s|found blocker below an excluded geometry %s\n",
        kind, IMPLEMENTATION_NAMES[implementation], record.getPath(), record.getHitFlags(),
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
    const long long queryNumber,
    const ReplayAnswer *answers)
{
    const ReplayAnswer captured(record.getBlockerPatchId(), record.getBlockerDistance());
    const ReplayAnswer &legacy = answers[LEGACY_VOXEL_GRID];
    if ( !legacy.identical(captured) ) {
        captureMismatches++;
        reportLine("capture mismatch", record, LEGACY_VOXEL_GRID, captured, legacy);
    }
    const bool closest = (record.getHitFlags() & RayHitFlag::ANY) == 0;
    const bool linear = usesLinear(queryNumber);
    for ( int implementation = LINEAR_REFERENCE; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++ ) {
        if ( implementation == LINEAR_REFERENCE && !linear ) {
            continue;
        }
        const ReplayAnswer &other = answers[implementation];
        const Implementation which = static_cast<Implementation>(implementation);
        if ( other.hasHit() != legacy.hasHit() ) {
            visibilityDisagreements[implementation]++;
            reportLine("visibility", record, which, legacy, other);
        } else if ( closest && !other.identical(legacy) ) {
            if ( other.hasHit() && other.sameDistance(legacy) ) {
                closestTies[implementation]++;
                reportLine("tie", record, which, legacy, other);
            } else {
                closestDisagreements[implementation]++;
                reportLine("closest hit", record, which, legacy, other);
            }
        }
    }
    if ( linear ) {
        const ReplayAnswer &reference = answers[LINEAR_REFERENCE];
        const ReplayAnswer &binned = answers[BINNED_SAH];
        if ( reference.hasHit() != binned.hasHit()
             || (closest && !binned.identical(reference) && !binned.sameDistance(reference)) ) {
            binnedVersusLinear++;
            reportLine("binned versus linear", record, BINNED_SAH, reference, binned);
        }
    }
}

void
RayCorpusReplay::replayChunk(
    const VisibilityQueryRecord *records,
    const int numberOfRecords,
    const long long firstQueryNumber)
{
    ReplayAnswer *sequential = new ReplayAnswer[NUMBER_OF_IMPLEMENTATIONS * numberOfRecords];

    // Sequential pass, one implementation at a time for the timings
    {
        VoxelGridWorkspace gridWorkspace(scene->voxelGrid->getNumberOfItems());
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        for ( int i = 0; i < numberOfRecords; i++ ) {
            sequential[NUMBER_OF_IMPLEMENTATIONS * i + LEGACY_VOXEL_GRID] = answerLegacy(records[i], gridWorkspace);
        }
        seconds[LEGACY_VOXEL_GRID] += secondsSince(start);

        SpatialIndexWorkspace binnedWorkspace(binnedIndex->getWorkspaceSize());
        start = std::chrono::steady_clock::now();
        for ( int i = 0; i < numberOfRecords; i++ ) {
            sequential[NUMBER_OF_IMPLEMENTATIONS * i + BINNED_SAH] = answerIndex(*binnedIndex, records[i], binnedWorkspace);
        }
        seconds[BINNED_SAH] += secondsSince(start);
        primitiveTests[BINNED_SAH] += binnedWorkspace.getPrimitiveTests();
        nodeVisits[BINNED_SAH] += binnedWorkspace.getNodeVisits();

        SpatialIndexWorkspace linearWorkspace(linearIndex->getWorkspaceSize());
        start = std::chrono::steady_clock::now();
        for ( int i = 0; i < numberOfRecords; i++ ) {
            if ( usesLinear(firstQueryNumber + i) ) {
                sequential[NUMBER_OF_IMPLEMENTATIONS * i + LINEAR_REFERENCE] = answerIndex(*linearIndex, records[i], linearWorkspace);
                linearReplayed++;
            }
        }
        seconds[LINEAR_REFERENCE] += secondsSince(start);
        primitiveTests[LINEAR_REFERENCE] += linearWorkspace.getPrimitiveTests();
    }

    for ( int i = 0; i < numberOfRecords; i++ ) {
        compare(records[i], firstQueryNumber + i, &sequential[NUMBER_OF_IMPLEMENTATIONS * i]);
    }

    // Threaded pass: the workers share the scene and the indexes, each with its own workspaces
    if ( numberOfThreads > 1 ) {
        ReplayAnswer *threaded = new ReplayAnswer[NUMBER_OF_IMPLEMENTATIONS * numberOfRecords];
        java::ArrayList<std::thread *> workers;
        for ( int t = 0; t < numberOfThreads; t++ ) {
            workers.add(new std::thread(
                &RayCorpusReplay::answerRange, this, records, numberOfRecords, firstQueryNumber, t, numberOfThreads, threaded));
        }
        for ( int t = 0; t < numberOfThreads; t++ ) {
            workers.get(t)->join();
            delete workers.get(t);
        }
        for ( int i = 0; i < numberOfRecords; i++ ) {
            for ( int implementation = 0; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++ ) {
                if ( implementation == LINEAR_REFERENCE && !usesLinear(firstQueryNumber + i) ) {
                    continue;
                }
                const ReplayAnswer &expected = sequential[NUMBER_OF_IMPLEMENTATIONS * i + implementation];
                const ReplayAnswer &found = threaded[NUMBER_OF_IMPLEMENTATIONS * i + implementation];
                if ( !found.identical(expected) ) {
                    threadMismatches++;
                    reportLine("thread mismatch", records[i], static_cast<Implementation>(implementation), expected, found);
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
    fprintf(stdout, "  faces %d, voxel grid items %d, threads %d, linear reference on 1 query in %lld\n",
        scene->getFaceBounds()->getNumberOfFaces(), scene->voxelGrid->getNumberOfItems(), numberOfThreads, linearStride);
    for ( int path = 0; path < NUMBER_OF_PATHS; path++ ) {
        fprintf(stdout, "  queries of path %d: %lld%s\n", path, queriesByPath[path], path == 0 ? " (restricted list, not replayed)" : "");
    }
    fprintf(stdout, "  replayed %lld (linear reference %lld), unsupported %lld\n", replayed, linearReplayed, unsupported);
    fprintf(stdout, "  legacy voxel grid against the capture: %lld mismatches\n", captureMismatches);
    for ( int implementation = LINEAR_REFERENCE; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++ ) {
        fprintf(stdout, "  %s against the legacy grid: visibility %lld, closest hit %lld, ties %lld\n",
            IMPLEMENTATION_NAMES[implementation], visibilityDisagreements[implementation],
            closestDisagreements[implementation], closestTies[implementation]);
    }
    fprintf(stdout, "  binned SAH against the linear reference: %lld disagreements\n", binnedVersusLinear);
    fprintf(stdout, "  threaded against sequential: %lld mismatches\n", threadMismatches);
    for ( int implementation = 0; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++ ) {
        fprintf(stdout, "  time %s: %.2f s, primitive tests %lld, node visits %lld\n",
            IMPLEMENTATION_NAMES[implementation], seconds[implementation],
            primitiveTests[implementation], nodeVisits[implementation]);
    }
    fprintf(stdout, "  %s\n", passed() ? "PASSED" : "FAILED");
}

bool
RayCorpusReplay::passed() const {
    bool ok = replayed > 0 && unsupported == 0 && captureMismatches == 0 && binnedVersusLinear == 0 && threadMismatches == 0;
    for ( int implementation = LINEAR_REFERENCE; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++ ) {
        ok = ok && visibilityDisagreements[implementation] == 0 && closestDisagreements[implementation] == 0;
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
    if ( reportFileName != nullptr && reportFileName[0] != '\0' ) {
        report = fopen(reportFileName, "w");
    }
    buildIndexes();

    VisibilityQueryRecord *records = new VisibilityQueryRecord[CHUNK_SIZE];
    int count = 0;
    VisibilityQueryRecord record;
    while ( (queryLimit <= 0 || replayed + count < queryLimit) && reader.next(&record) ) {
        const int path = record.getPath();
        if ( path < 0 || path >= NUMBER_OF_PATHS ) {
            unsupported++;
            continue;
        }
        queriesByPath[path]++;
        if ( path == 0 ) {
            continue;
        }
        if ( !isSupported(record) ) {
            unsupported++;
            continue;
        }
        records[count++] = record;
        if ( count == CHUNK_SIZE ) {
            replayChunk(records, count, replayed);
            replayed += count;
            count = 0;
            fprintf(stderr, "Ray corpus replay: %lld queries\n", replayed);
        }
    }
    if ( count > 0 ) {
        replayChunk(records, count, replayed);
        replayed += count;
    }
    delete[] records;
    if ( !reader.isValid() ) {
        fprintf(stderr, "Ray corpus replay: %s is malformed after %lld queries\n", corpusFileName, replayed);
        return 2;
    }
    printSummary();
    return passed() ? 0 : 1;
}

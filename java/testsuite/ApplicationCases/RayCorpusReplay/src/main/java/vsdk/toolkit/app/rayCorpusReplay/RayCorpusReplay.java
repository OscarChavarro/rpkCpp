package vsdk.toolkit.app.rayCorpusReplay;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.math.BigDecimal;
import java.math.MathContext;
import java.math.RoundingMode;
import java.util.ArrayList;
import java.util.Locale;
import vsdk.toolkit.app.SceneConsumer;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.environment.geometry.elements.RayHit;
import vsdk.toolkit.environment.geometry.elements.RayHitFlag;
import vsdk.toolkit.environment.geometry.spatialIndex.GridSpatialIndex;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialIndex;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialIndexWorkspace;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialQueryMode;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialQueryResult;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.scene.SceneAccelerationMethod;
import vsdk.toolkit.scene.SceneFaceRayTest;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.scene.VoxelGridWorkspace;

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
compared. The replay is then repeated from several threads sharing the scene
and the indexes, one workspace each, and every answer must equal the
sequential one bit for bit.

Restricted-list queries (path 0) are counted and skipped: they do not reach
a scene index, and the shaft culling patch sets they name exist only during
the run that wrote them. Geometry exclusions (Galerkin full-scene path) are
mapped to the geometries of the cluster hierarchy (FaceAncestry); a query
naming one the scene does not have is counted as unsupported.

Every disagreement is written, with its ray, to the report file, for the
owner to decide (plan decision D5). The exit status is non-zero on any
capture mismatch, disagreement other than a tie, thread mismatch or
unsupported query.

Java adaptations: the per-query values the C++ replay keeps on the stack
live in one Worker per thread; answers are kept in primitive arrays
(ReplayAnswerTable); the report numbers are printed as C's %.9g. The
threaded pass is off by default (one thread), because the Java face test
(Patch.intersect and the box tests it relies on) reuses static scratch
objects and is not safe to call from several threads; RPK_RAY_REPLAY_THREADS
above 1 runs it anyway and then reports the resulting thread mismatches.
*/
public final class RayCorpusReplay implements SceneConsumer {
    public static final int LEGACY_VOXEL_GRID = 0;
    public static final int LINEAR_REFERENCE = 1;
    public static final int BINNED_SAH = 2;
    public static final int PORTED_GRID = 3;
    public static final int NUMBER_OF_IMPLEMENTATIONS = 4;

    private static final int NUMBER_OF_PATHS = 5;

    // Queries read and answered at a time; the threaded pass repeats each chunk
    private static final int CHUNK_SIZE = 1 << 20;

    // Disagreement lines written to the report, per run
    private static final long MAXIMUM_REPORTED_LINES = 100000;

    private static final String[] IMPLEMENTATION_NAMES = {
        "legacy voxel grid",
        "linear reference",
        "binned SAH (WALD2007)",
        "ported voxel grid"
    };

    private final String corpusFileName;
    private final String reportFileName;
    private final int numberOfThreads;
    private final long linearStride;
    private final long queryLimit;

    private Scene scene;
    private final ArrayList<Patch> patchById;
    private FaceAncestry ancestry;
    private VoxelGrid legacyGrid;
    private SpatialIndex linearIndex;
    private SpatialIndex binnedIndex;
    private SpatialIndex gridIndex;
    private PrintStream report;

    private final long[] queriesByPath;
    private long replayed;
    private long linearReplayed;
    private long unsupported;
    private long captureMismatches;
    private final long[] visibilityDisagreements;
    private final long[] closestDisagreements;
    private final long[] closestTies;
    private long binnedVersusLinear;
    private long gridBlockerDifferences;
    private long threadMismatches;
    private long reportedLines;
    private final long[] primitiveTests;
    private final long[] nodeVisits;
    private final double[] seconds;

    /**
    The query scratch of one thread: the C++ replay's stack values and its
    workspaces of the indexes
    */
    private final class Worker {
        private final Ray ray;
        private final RayHit hitStore;
        private final float[] distance;
        private final SceneFaceRayTest test;
        private final vsdk.toolkit.environment.geometry.element.Ray indexRay;
        private final VoxelGridWorkspace gridWorkspace;
        private final SpatialIndexWorkspace linearWorkspace;
        private final SpatialIndexWorkspace binnedWorkspace;
        private final SpatialIndexWorkspace portedWorkspace;

        private Worker() {
            ray = new Ray();
            hitStore = new RayHit();
            distance = new float[1];
            test = new SceneFaceRayTest(scene, null, 0, null, null);
            indexRay = vsdk.toolkit.environment.geometry.element.Ray.fromAffine(
                new Vector3Dd(0, 0, 0), new Vector3Dd(1, 0, 0));
            gridWorkspace = new VoxelGridWorkspace(legacyGrid.getNumberOfItems(), legacyGrid.getNumberOfLevels());
            linearWorkspace = new SpatialIndexWorkspace(linearIndex.getWorkspaceSize());
            binnedWorkspace = new SpatialIndexWorkspace(binnedIndex.getWorkspaceSize());
            portedWorkspace = new SpatialIndexWorkspace(gridIndex.getWorkspaceSize());
        }

        private void setRay(VisibilityQueryRecord record) {
            ray.position.set(record.getOrigin(0), record.getOrigin(1), record.getOrigin(2));
            ray.direction.set(record.getDirection(0), record.getDirection(1), record.getDirection(2));
        }
    }

    public RayCorpusReplay(
        String inCorpusFileName,
        String inReportFileName,
        int inNumberOfThreads,
        long inLinearStride,
        long inQueryLimit) {
        corpusFileName = inCorpusFileName;
        reportFileName = inReportFileName;
        numberOfThreads = inNumberOfThreads > 0 ? inNumberOfThreads : 1;
        linearStride = inLinearStride;
        queryLimit = inQueryLimit;
        scene = null;
        patchById = new ArrayList<>();
        ancestry = null;
        legacyGrid = null;
        linearIndex = null;
        binnedIndex = null;
        gridIndex = null;
        report = null;
        queriesByPath = new long[NUMBER_OF_PATHS];
        replayed = 0;
        linearReplayed = 0;
        unsupported = 0;
        captureMismatches = 0;
        visibilityDisagreements = new long[NUMBER_OF_IMPLEMENTATIONS];
        closestDisagreements = new long[NUMBER_OF_IMPLEMENTATIONS];
        closestTies = new long[NUMBER_OF_IMPLEMENTATIONS];
        binnedVersusLinear = 0;
        gridBlockerDifferences = 0;
        threadMismatches = 0;
        reportedLines = 0;
        primitiveTests = new long[NUMBER_OF_IMPLEMENTATIONS];
        nodeVisits = new long[NUMBER_OF_IMPLEMENTATIONS];
        seconds = new double[NUMBER_OF_IMPLEMENTATIONS];
    }

    /**
    Releases the indexes and closes the report (the C++ destructor)
    */
    public void destroy() {
        if (legacyGrid != null) {
            legacyGrid.freeVoxelGridElements();
            legacyGrid = null;
        }
        linearIndex = null;
        binnedIndex = null;
        gridIndex = null;
        ancestry = null;
        if (report != null) {
            report.close();
            report = null;
        }
    }

    private static double secondsSince(long start) {
        return (double)(System.nanoTime() - start) / 1000000000.0;
    }

    /**
    C's %.{precision}g of a value: the shortest of the fixed and exponent
    forms that the C library chooses, without trailing zeros
    */
    private static String formatG(double value, int precision) {
        if (Double.isNaN(value)) {
            return "nan";
        }
        if (Double.isInfinite(value)) {
            return value > 0 ? "inf" : "-inf";
        }
        if (value == 0.0) {
            return (Double.doubleToRawLongBits(value) < 0) ? "-0" : "0";
        }
        BigDecimal rounded = new BigDecimal(value).round(new MathContext(precision, RoundingMode.HALF_EVEN));
        final int exponent = rounded.precision() - rounded.scale() - 1;
        if (exponent < -4 || exponent >= precision) {
            BigDecimal mantissa = rounded.movePointLeft(exponent);
            String digits = stripZeros(mantissa.toPlainString());
            return String.format(Locale.US, "%se%s%02d", digits, exponent < 0 ? "-" : "+", Math.abs(exponent));
        }
        return stripZeros(rounded.setScale(Math.max(precision - 1 - exponent, 0), RoundingMode.HALF_EVEN).toPlainString());
    }

    private static String stripZeros(String number) {
        if (number.indexOf('.') < 0) {
            return number;
        }
        int end = number.length();
        while (number.charAt(end - 1) == '0') {
            end--;
        }
        if (number.charAt(end - 1) == '.') {
            end--;
        }
        return number.substring(0, end);
    }

    /**
    The legacy grid as the scene builder built it before the scene query, and
    the VITRAL indexes as the scene query builds them (SceneQuery.buildIndex):
    primitive i is face i of the scene's face table.
    */
    private void buildIndexes() {
        final int numberOfFaces = scene.getNumberOfFaces();
        ancestry = new FaceAncestry(scene);
        legacyGrid = new VoxelGrid(scene.clusteredRootGeometry);
        linearIndex = SceneQuery.buildIndex(scene, SceneAccelerationMethod.BRUTE_FORCE_ACCELERATION);
        binnedIndex = SceneQuery.buildIndex(scene, SceneAccelerationMethod.WALD2007_ACCELERATION);
        gridIndex = SceneQuery.buildIndex(scene, SceneAccelerationMethod.VOXEL_ACCELERATION);

        int maximumId = 0;
        for (int i = 0; i < numberOfFaces; i++) {
            final int id = scene.getFace(i).id;
            maximumId = id > maximumId ? id : maximumId;
        }
        for (int id = 0; id <= maximumId; id++) {
            patchById.add(null);
        }
        for (int i = 0; i < numberOfFaces; i++) {
            Patch patch = scene.getFace(i);
            patchById.set(patch.id, patch);
        }
    }

    private boolean isSupported(VisibilityQueryRecord record) {
        for (int i = 0; i < VisibilityQueryRecord.NUMBER_OF_EXCLUDED_GEOMETRIES; i++) {
            final int id = record.getExcludedGeometryId(i);
            if (id != -1 && ancestry.getGeometry(id) == null) {
                return false;
            }
        }
        for (int i = 0; i < VisibilityQueryRecord.NUMBER_OF_EXCLUDED_PATCHES; i++) {
            final int id = record.getExcludedPatchId(i);
            if (id < 0 || id >= patchById.size() || (id != 0 && patchById.get(id) == null)) {
                return false;
            }
        }
        return true;
    }

    private boolean usesLinear(long queryNumber) {
        return linearStride > 0 && queryNumber % linearStride == 0;
    }

    private RayExclusionSet recordExclusions(VisibilityQueryRecord record) {
        Patch[] excluded = new Patch[VisibilityQueryRecord.NUMBER_OF_EXCLUDED_PATCHES];
        for (int i = 0; i < VisibilityQueryRecord.NUMBER_OF_EXCLUDED_PATCHES; i++) {
            final int id = record.getExcludedPatchId(i);
            excluded[i] = id != 0 ? patchById.get(id) : null;
        }
        final int geometry1 = record.getExcludedGeometryId(0);
        final int geometry2 = record.getExcludedGeometryId(1);
        return new RayExclusionSet(
            excluded[0], excluded[1], excluded[2], excluded[3],
            geometry1 != -1 ? ancestry.getGeometry(geometry1) : null,
            geometry2 != -1 ? ancestry.getGeometry(geometry2) : null);
    }

    private void answerLegacy(VisibilityQueryRecord record, Worker worker, ReplayAnswerTable answers, int slot) {
        worker.setRay(record);
        RayExclusionSet exclusions = recordExclusions(record);
        worker.distance[0] = record.getMaximumDistance();
        RayHit hit = legacyGrid.gridIntersect(
            worker.ray, record.getMinimumDistance(), worker.distance, record.getHitFlags(),
            worker.hitStore, exclusions, worker.gridWorkspace);
        if (hit == null) {
            answers.set(slot, 0, record.getMaximumDistance());
            return;
        }
        answers.set(slot, hit.getPatch().id, worker.distance[0]);
    }

    private void answerIndex(
        SpatialIndex index,
        VisibilityQueryRecord record,
        Worker worker,
        SpatialIndexWorkspace workspace,
        ReplayAnswerTable answers,
        int slot) {
        worker.setRay(record);
        RayExclusionSet exclusions = recordExclusions(record);
        worker.test.set(worker.ray, record.getHitFlags(), exclusions, worker.hitStore);
        worker.indexRay.setOriginAndDirection(
            new Vector3Dd(worker.ray.position.x, worker.ray.position.y, worker.ray.position.z),
            new Vector3Dd(worker.ray.direction.x, worker.ray.direction.y, worker.ray.direction.z));
        worker.indexRay.setT(0.0);
        final SpatialQueryMode mode = (record.getHitFlags() & RayHitFlag.ANY) != 0
            ? SpatialQueryMode.ANY_HIT : SpatialQueryMode.CLOSEST_HIT;
        final SpatialQueryResult result = index.query(
            worker.indexRay, record.getMinimumDistance(), record.getMaximumDistance(), mode, worker.test, workspace);
        if (!result.hasHit()) {
            answers.set(slot, 0, record.getMaximumDistance());
            return;
        }
        answers.set(slot, scene.getFace(result.getPrimitive()).id, (float)result.getParameter());
    }

    /**
    Answers records first, first + step, ... with every implementation (the
    linear reference only on its sampled queries) into the slots
    NUMBER_OF_IMPLEMENTATIONS * record + implementation. Used by each worker
    of the threaded pass, with its own workspaces.
    */
    private void answerRange(
        VisibilityQueryRecord[] records,
        int numberOfRecords,
        long firstQueryNumber,
        int first,
        int step,
        ReplayAnswerTable answers) {
        Worker worker = new Worker();
        for (int i = first; i < numberOfRecords; i += step) {
            final int base = NUMBER_OF_IMPLEMENTATIONS * i;
            answerLegacy(records[i], worker, answers, base + LEGACY_VOXEL_GRID);
            answerIndex(binnedIndex, records[i], worker, worker.binnedWorkspace, answers, base + BINNED_SAH);
            answerIndex(gridIndex, records[i], worker, worker.portedWorkspace, answers, base + PORTED_GRID);
            if (usesLinear(firstQueryNumber + i)) {
                answerIndex(linearIndex, records[i], worker, worker.linearWorkspace, answers, base + LINEAR_REFERENCE);
            }
        }
    }

    private boolean belowExcludedGeometry(VisibilityQueryRecord record, ReplayAnswer answer) {
        if (!answer.hasHit() || answer.getBlockerPatchId() >= patchById.size()) {
            return false;
        }
        Patch blocker = patchById.get(answer.getBlockerPatchId());
        return blocker != null && ancestry.excludesFace(blocker.getFaceIndex(), recordExclusions(record));
    }

    private void reportLine(
        String kind,
        VisibilityQueryRecord record,
        int implementation,
        ReplayAnswer expected,
        ReplayAnswer found) {
        if (report == null || reportedLines >= MAXIMUM_REPORTED_LINES) {
            return;
        }
        reportedLines++;
        StringBuilder line = new StringBuilder(512);
        line.append(kind).append('|').append(IMPLEMENTATION_NAMES[implementation])
            .append("|path ").append(record.getPath())
            .append("|flags 0x").append(Integer.toHexString(record.getHitFlags()))
            .append("|origin ").append(formatG(record.getOrigin(0), 9))
            .append(' ').append(formatG(record.getOrigin(1), 9))
            .append(' ').append(formatG(record.getOrigin(2), 9))
            .append("|direction ").append(formatG(record.getDirection(0), 9))
            .append(' ').append(formatG(record.getDirection(1), 9))
            .append(' ').append(formatG(record.getDirection(2), 9))
            .append("|interval ").append(formatG(record.getMinimumDistance(), 9))
            .append(' ').append(formatG(record.getMaximumDistance(), 9))
            .append("|excluded ").append(record.getExcludedPatchId(0))
            .append(' ').append(record.getExcludedPatchId(1))
            .append(' ').append(record.getExcludedPatchId(2))
            .append(' ').append(record.getExcludedPatchId(3))
            .append("|excluded geometries ").append(record.getExcludedGeometryId(0))
            .append(' ').append(record.getExcludedGeometryId(1))
            .append("|expected ").append(expected.getBlockerPatchId())
            .append(' ').append(formatG(expected.getDistance(), 9))
            .append("|found ").append(found.getBlockerPatchId())
            .append(' ').append(formatG(found.getDistance(), 9))
            .append("|expected blocker below an excluded geometry ")
            .append(belowExcludedGeometry(record, expected) ? "yes" : "no")
            .append("|found blocker below an excluded geometry ")
            .append(belowExcludedGeometry(record, found) ? "yes" : "no")
            .append('\n');
        report.print(line);
    }

    /**
    Compares the sequential answers of one query: the legacy grid with the
    capture, then each index with the legacy grid, and the binned index with
    the linear reference.
    */
    private void compare(VisibilityQueryRecord record, long queryNumber, ReplayAnswerTable answers, int base) {
        final ReplayAnswer captured = new ReplayAnswer(record.getBlockerPatchId(), record.getBlockerDistance());
        final ReplayAnswer legacy = answers.get(base + LEGACY_VOXEL_GRID);
        if (!legacy.identical(captured)) {
            captureMismatches++;
            reportLine("capture mismatch", record, LEGACY_VOXEL_GRID, captured, legacy);
        }
        final boolean closest = (record.getHitFlags() & RayHitFlag.ANY) == 0;
        final boolean linear = usesLinear(queryNumber);
        for (int implementation = LINEAR_REFERENCE; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++) {
            if (implementation == LINEAR_REFERENCE && !linear) {
                continue;
            }
            final ReplayAnswer other = answers.get(base + implementation);
            if (other.hasHit() != legacy.hasHit()) {
                visibilityDisagreements[implementation]++;
                reportLine("visibility", record, implementation, legacy, other);
            }
            else if (closest && !other.identical(legacy)) {
                if (other.hasHit() && other.sameDistance(legacy)) {
                    closestTies[implementation]++;
                    reportLine("tie", record, implementation, legacy, other);
                }
                else {
                    closestDisagreements[implementation]++;
                    reportLine("closest hit", record, implementation, legacy, other);
                }
            }
        }
        // The ported grid follows the legacy traversal order, any-hit included
        final ReplayAnswer ported = answers.get(base + PORTED_GRID);
        if (!ported.identical(legacy)) {
            gridBlockerDifferences++;
            if (ported.hasHit() == legacy.hasHit() && (!closest || ported.sameDistance(legacy))) {
                reportLine("ported grid blocker", record, PORTED_GRID, legacy, ported);
            }
        }
        if (linear) {
            final ReplayAnswer reference = answers.get(base + LINEAR_REFERENCE);
            final ReplayAnswer binned = answers.get(base + BINNED_SAH);
            if (reference.hasHit() != binned.hasHit()
                || (closest && !binned.identical(reference) && !binned.sameDistance(reference))) {
                binnedVersusLinear++;
                reportLine("binned versus linear", record, BINNED_SAH, reference, binned);
            }
        }
    }

    private void replayChunk(VisibilityQueryRecord[] records, int numberOfRecords, long firstQueryNumber) {
        ReplayAnswerTable sequential = new ReplayAnswerTable(NUMBER_OF_IMPLEMENTATIONS * numberOfRecords);

        // Sequential pass, one implementation at a time for the timings
        Worker worker = new Worker();
        long start = System.nanoTime();
        for (int i = 0; i < numberOfRecords; i++) {
            answerLegacy(records[i], worker, sequential, NUMBER_OF_IMPLEMENTATIONS * i + LEGACY_VOXEL_GRID);
        }
        seconds[LEGACY_VOXEL_GRID] += secondsSince(start);

        start = System.nanoTime();
        for (int i = 0; i < numberOfRecords; i++) {
            answerIndex(binnedIndex, records[i], worker, worker.binnedWorkspace, sequential,
                NUMBER_OF_IMPLEMENTATIONS * i + BINNED_SAH);
        }
        seconds[BINNED_SAH] += secondsSince(start);
        primitiveTests[BINNED_SAH] += worker.binnedWorkspace.getPrimitiveTests();
        nodeVisits[BINNED_SAH] += worker.binnedWorkspace.getNodeVisits();

        start = System.nanoTime();
        for (int i = 0; i < numberOfRecords; i++) {
            answerIndex(gridIndex, records[i], worker, worker.portedWorkspace, sequential,
                NUMBER_OF_IMPLEMENTATIONS * i + PORTED_GRID);
        }
        seconds[PORTED_GRID] += secondsSince(start);
        primitiveTests[PORTED_GRID] += worker.portedWorkspace.getPrimitiveTests();
        nodeVisits[PORTED_GRID] += worker.portedWorkspace.getNodeVisits();

        start = System.nanoTime();
        for (int i = 0; i < numberOfRecords; i++) {
            if (usesLinear(firstQueryNumber + i)) {
                answerIndex(linearIndex, records[i], worker, worker.linearWorkspace, sequential,
                    NUMBER_OF_IMPLEMENTATIONS * i + LINEAR_REFERENCE);
                linearReplayed++;
            }
        }
        seconds[LINEAR_REFERENCE] += secondsSince(start);
        primitiveTests[LINEAR_REFERENCE] += worker.linearWorkspace.getPrimitiveTests();

        for (int i = 0; i < numberOfRecords; i++) {
            compare(records[i], firstQueryNumber + i, sequential, NUMBER_OF_IMPLEMENTATIONS * i);
        }

        // Threaded pass: the workers share the scene and the indexes, each with its own workspaces
        if (numberOfThreads > 1) {
            ReplayAnswerTable threaded = new ReplayAnswerTable(NUMBER_OF_IMPLEMENTATIONS * numberOfRecords);
            ArrayList<Thread> workers = new ArrayList<>();
            for (int t = 0; t < numberOfThreads; t++) {
                final int first = t;
                Thread thread = new Thread(
                    () -> answerRange(records, numberOfRecords, firstQueryNumber, first, numberOfThreads, threaded));
                workers.add(thread);
                thread.start();
            }
            boolean interrupted = false;
            for (Thread thread : workers) {
                boolean joined = false;
                while (!joined) {
                    try {
                        thread.join();
                        joined = true;
                    }
                    catch (InterruptedException e) {
                        interrupted = true;
                    }
                }
            }
            if (interrupted) {
                Thread.currentThread().interrupt();
            }
            for (int i = 0; i < numberOfRecords; i++) {
                for (int implementation = 0; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++) {
                    if (implementation == LINEAR_REFERENCE && !usesLinear(firstQueryNumber + i)) {
                        continue;
                    }
                    final int slot = NUMBER_OF_IMPLEMENTATIONS * i + implementation;
                    final ReplayAnswer expected = sequential.get(slot);
                    final ReplayAnswer found = threaded.get(slot);
                    if (!found.identical(expected)) {
                        threadMismatches++;
                        reportLine("thread mismatch", records[i], implementation, expected, found);
                    }
                }
            }
        }
    }

    private void printSummary() {
        PrintStream out = System.out;
        out.printf(Locale.US, "Ray corpus replay: %s\n", corpusFileName);
        out.printf(Locale.US, "  faces %d, voxel grid items %d, threads %d, linear reference on 1 query in %d\n",
            scene.getFaceBounds().getNumberOfFaces(), legacyGrid.getNumberOfItems(), numberOfThreads, linearStride);
        for (int path = 0; path < NUMBER_OF_PATHS; path++) {
            out.printf(Locale.US, "  queries of path %d: %d%s\n", path, queriesByPath[path],
                path == 0 ? " (restricted list, not replayed)" : "");
        }
        out.printf(Locale.US, "  replayed %d (linear reference %d), unsupported %d\n", replayed, linearReplayed, unsupported);
        out.printf(Locale.US, "  legacy voxel grid against the capture: %d mismatches\n", captureMismatches);
        for (int implementation = LINEAR_REFERENCE; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++) {
            out.printf(Locale.US, "  %s against the legacy grid: visibility %d, closest hit %d, ties %d\n",
                IMPLEMENTATION_NAMES[implementation], visibilityDisagreements[implementation],
                closestDisagreements[implementation], closestTies[implementation]);
        }
        out.printf(Locale.US, "  binned SAH against the linear reference: %d disagreements\n", binnedVersusLinear);
        out.printf(Locale.US,
            "  ported voxel grid: %d items, %d grids; blocker or distance other than the legacy grid's (any query): %d\n",
            ((GridSpatialIndex)gridIndex).getNumberOfItems(),
            ((GridSpatialIndex)gridIndex).getNumberOfGrids(), gridBlockerDifferences);
        out.printf(Locale.US, "  threaded against sequential: %d mismatches\n", threadMismatches);
        for (int implementation = 0; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++) {
            out.printf(Locale.US, "  time %s: %.2f s, primitive tests %d, node visits %d\n",
                IMPLEMENTATION_NAMES[implementation], seconds[implementation],
                primitiveTests[implementation], nodeVisits[implementation]);
        }
        out.printf(Locale.US, "  %s\n", passed() ? "PASSED" : "FAILED");
        out.flush();
    }

    private boolean passed() {
        boolean ok = replayed > 0 && unsupported == 0 && captureMismatches == 0
            && binnedVersusLinear == 0 && threadMismatches == 0;
        for (int implementation = LINEAR_REFERENCE; implementation < NUMBER_OF_IMPLEMENTATIONS; implementation++) {
            ok = ok && visibilityDisagreements[implementation] == 0 && closestDisagreements[implementation] == 0;
        }
        return ok;
    }

    @Override
    public int consume(Scene inScene) {
        scene = inScene;
        RayCorpusReader reader = new RayCorpusReader(corpusFileName);
        if (!reader.isValid()) {
            System.err.printf("Ray corpus replay: %s is not a readable ray corpus\n", corpusFileName);
            reader.close();
            return 2;
        }
        if (reportFileName != null && !reportFileName.isEmpty()) {
            try {
                report = new PrintStream(new FileOutputStream(reportFileName), false, "UTF-8");
            }
            catch (IOException e) {
                report = null;
            }
        }
        buildIndexes();

        VisibilityQueryRecord[] records = new VisibilityQueryRecord[CHUNK_SIZE];
        for (int i = 0; i < CHUNK_SIZE; i++) {
            records[i] = new VisibilityQueryRecord();
        }
        int count = 0;
        while ((queryLimit <= 0 || replayed + count < queryLimit) && reader.next(records[count])) {
            VisibilityQueryRecord record = records[count];
            final int path = record.getPath();
            if (path < 0 || path >= NUMBER_OF_PATHS) {
                unsupported++;
                continue;
            }
            queriesByPath[path]++;
            if (path == 0) {
                continue;
            }
            if (!isSupported(record)) {
                unsupported++;
                continue;
            }
            count++;
            if (count == CHUNK_SIZE) {
                replayChunk(records, count, replayed);
                replayed += count;
                count = 0;
                System.err.printf("Ray corpus replay: %d queries\n", replayed);
            }
        }
        if (count > 0) {
            replayChunk(records, count, replayed);
            replayed += count;
        }
        final boolean valid = reader.isValid();
        reader.close();
        if (!valid) {
            System.err.printf("Ray corpus replay: %s is malformed after %d queries\n", corpusFileName, replayed);
            return 2;
        }
        printSummary();
        return passed() ? 0 : 1;
    }
}

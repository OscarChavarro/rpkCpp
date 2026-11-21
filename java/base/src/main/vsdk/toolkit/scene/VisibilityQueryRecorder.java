package vsdk.toolkit.scene;

import java.io.BufferedOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.environment.geometry.elements.RayHit;
import vsdk.toolkit.skin.Geometry;

/**
Ray accounting and ray corpus capture of the visibility queries of one run.
Only runs with ray capture enabled (captureEnabled(): the RPK_RAY_CAPTURE
environment variable or the rpk.rayCapture system property, the Java
counterpart of the C++ WITH_RAY_CAPTURE build) create one, so a default run
is unchanged.

It counts where each shadow ray of the Galerkin solver is answered (shadow
cache, restricted geometry list produced by shaft culling, or full-scene
index) and writes every list and index query, with its answer, to a binary
corpus file that later index implementations are replayed against.

Corpus format, all fields 32-bit, little-endian (the host byte order of the
C++ reference machines, so that both ports read each other's corpora):

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
shooting. Patch identifiers are Patch.id (from 1), geometry identifiers
Geometry.id, both deterministic for a given scene and options.

Workspace role: owned by the scene, reached through the caller's
SceneQueryWorkspace; one per worker if ever used concurrently.

Java adaptation: the C++ destructor's summary and file close are close(),
which Scene.destroy() and a shutdown hook registered by the scene builder
call; floats are written with their raw bits.
*/
public final class VisibilityQueryRecorder {
    public static final int GALERKIN_RESTRICTED_LIST = 0;
    public static final int GALERKIN_FULL_SCENE = 1;
    public static final int RAYTRACER_CLOSEST_HIT = 2;
    public static final int RAYTRACER_VISIBILITY = 3;
    public static final int STOCHASTIC_SHOOTING = 4;
    public static final int NUMBER_OF_PATHS = 5;

    private static final int TAG_GEOMETRY_LIST = 1;
    private static final int TAG_QUERY = 2;
    private static final int QUERY_RECORD_BYTES = 4 * 20;
    private static final int OUTPUT_BUFFER_SIZE = 1 << 20;

    private OutputStream corpus;
    private final String corpusFileName;
    private final ByteBuffer scratch;
    private long cacheResolved;
    private final long[] queries;
    private final long[] hits;
    private long geometryLists;
    private boolean closed;

    /**
    True when this run captures rays: the system property rpk.rayCapture is
    true, or the environment variable RPK_RAY_CAPTURE is ON, 1, TRUE or YES
    (any case)
    */
    public static boolean captureEnabled() {
        if (Boolean.parseBoolean(System.getProperty("rpk.rayCapture"))) {
            return true;
        }
        String value = System.getenv("RPK_RAY_CAPTURE");
        if (value == null) {
            return false;
        }
        value = value.trim();
        return value.equalsIgnoreCase("ON") || value.equals("1")
            || value.equalsIgnoreCase("TRUE") || value.equalsIgnoreCase("YES");
    }

    private static String pathName(int path) {
        switch (path) {
            case GALERKIN_RESTRICTED_LIST:
                return "Galerkin restricted list";
            case GALERKIN_FULL_SCENE:
                return "Galerkin full-scene index";
            case RAYTRACER_CLOSEST_HIT:
                return "raytracer closest hit";
            case RAYTRACER_VISIBILITY:
                return "raytracer visibility";
            default:
                return "stochastic shooting";
        }
    }

    /**
    @param inCorpusFileName the corpus file to write, or null or empty to
    only count
    */
    public VisibilityQueryRecorder(String inCorpusFileName) {
        corpus = null;
        cacheResolved = 0;
        queries = new long[NUMBER_OF_PATHS];
        hits = new long[NUMBER_OF_PATHS];
        geometryLists = 0;
        closed = false;
        scratch = ByteBuffer.allocate(QUERY_RECORD_BYTES).order(ByteOrder.LITTLE_ENDIAN);
        if (inCorpusFileName != null && !inCorpusFileName.isEmpty()) {
            corpusFileName = inCorpusFileName;
            try {
                corpus = new BufferedOutputStream(new FileOutputStream(corpusFileName), OUTPUT_BUFFER_SIZE);
                scratch.clear();
                scratch.put("RPKVQC01".getBytes(StandardCharsets.US_ASCII));
                scratch.putInt(1);
                flushScratch();
            }
            catch (IOException e) {
                System.err.printf("Ray capture: cannot open corpus file %s, only counting\n", corpusFileName);
                corpus = null;
            }
        }
        else {
            corpusFileName = null;
        }
    }

    private void flushScratch() throws IOException {
        corpus.write(scratch.array(), 0, scratch.position());
        scratch.clear();
    }

    private void writeFailed(IOException e) {
        System.err.printf("Ray capture: cannot write corpus file %s (%s), only counting\n", corpusFileName, e.getMessage());
        try {
            corpus.close();
        }
        catch (IOException closeFailure) {
            e.addSuppressed(closeFailure);
        }
        corpus = null;
    }

    public void countCacheResolved() {
        cacheResolved++;
    }

    public void recordGeometryList(ArrayList<Geometry> geometryList) {
        geometryLists++;
        if (corpus == null) {
            return;
        }
        final int count = geometryList != null ? geometryList.size() : 0;
        try {
            scratch.clear();
            scratch.putInt(TAG_GEOMETRY_LIST);
            scratch.putInt(count);
            for (int i = 0; i < count; i++) {
                if (scratch.remaining() < 4) {
                    flushScratch();
                }
                scratch.putInt(geometryList.get(i).id);
            }
            flushScratch();
        }
        catch (IOException e) {
            writeFailed(e);
        }
    }

    public void recordQuery(
        int path,
        Ray ray,
        float minimumDistance,
        float maximumDistance,
        int hitFlags,
        RayExclusionSet exclusions,
        RayHit hit,
        float hitDistance) {
        queries[path]++;
        if (hit != null) {
            hits[path]++;
        }
        if (corpus == null) {
            return;
        }
        scratch.clear();
        scratch.putInt(TAG_QUERY);
        scratch.putInt(path);
        scratch.putInt(hitFlags);
        scratch.putFloat(ray.position.x);
        scratch.putFloat(ray.position.y);
        scratch.putFloat(ray.position.z);
        scratch.putFloat(ray.direction.x);
        scratch.putFloat(ray.direction.y);
        scratch.putFloat(ray.direction.z);
        scratch.putFloat(minimumDistance);
        scratch.putFloat(maximumDistance);
        for (int i = 0; i < RayExclusionSet.MAX_EXCLUDED_PATCHES; i++) {
            Patch patch = exclusions.getExcludedPatch(i);
            scratch.putInt(patch != null ? patch.id : 0);
        }
        scratch.putInt(exclusions.getExcludedGeometry1() != null ? exclusions.getExcludedGeometry1().id : -1);
        scratch.putInt(exclusions.getExcludedGeometry2() != null ? exclusions.getExcludedGeometry2().id : -1);
        Patch blocker = hit != null ? hit.getPatch() : null;
        scratch.putInt(blocker != null ? blocker.id : 0);
        scratch.putFloat(hit != null ? hitDistance : maximumDistance);
        try {
            flushScratch();
        }
        catch (IOException e) {
            writeFailed(e);
        }
    }

    public void printSummary() {
        System.err.printf("Ray capture: shadow rays resolved by the shadow cache: %d\n", cacheResolved);
        for (int i = 0; i < NUMBER_OF_PATHS; i++) {
            System.err.printf("Ray capture: %s queries: %d, with hit: %d\n", pathName(i), queries[i], hits[i]);
        }
        System.err.printf("Ray capture: restricted geometry lists: %d\n", geometryLists);
        if (corpusFileName != null) {
            System.err.printf("Ray capture: corpus written to %s\n", corpusFileName);
        }
    }

    /**
    Prints the summary and closes the corpus file; later calls do nothing
    */
    public synchronized void close() {
        if (closed) {
            return;
        }
        closed = true;
        printSummary();
        if (corpus != null) {
            try {
                corpus.close();
            }
            catch (IOException e) {
                System.err.printf("Ray capture: cannot close corpus file %s (%s)\n", corpusFileName, e.getMessage());
            }
            corpus = null;
        }
        System.err.flush();
    }
}

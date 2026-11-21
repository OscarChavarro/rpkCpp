package vsdk.toolkit.common.statistics;

import java.util.concurrent.atomic.AtomicLong;

/**
Counters of the CPU ray tracer, enabled with the system property
`vsdk.raytrace.stats` (read the first time the statistics are used) or with
`setEnabled`. Disabled, every `record` call is one test of a flag.

Acceleration structure counters follow the two levels of
`doc/flatBinaryTreeContract.md`, with the same meaning as the GPU counters
of the Vulkan 1.4 compute path (`VulkanComputeTraversalCounters`):

- instance level: hierarchies whose leaves hold meshes or transformed
  instances (`BinnedSahHierarchy.traverseClosest`, used by
  `TriangleMeshGroup` and `BinnedSahInstanceHierarchy`);
- primitive level: hierarchies whose leaves hold triangles
  (`BinnedSahTriangleHierarchy`), and the brute force loop (no nodes, every
  triangle tested).

A node visit is a node taken from the traversal stack and not skipped (its
box was entered before the closest hit); an entry / primitive test is one
leaf element handed to its test.

Counters are atomic, so parallel tracing gives exact totals; traversals add
their counts once per query, not once per node.
*/
public final class RaytraceStatistics {
    private static final AtomicLong primaryRays = new AtomicLong(0);
    private static final AtomicLong shadowRays = new AtomicLong(0);
    private static final AtomicLong reflectionRays = new AtomicLong(0);
    private static final AtomicLong sceneTraversals = new AtomicLong(0);
    private static final AtomicLong objectIntersectionTests = new AtomicLong(0);
    private static final AtomicLong raysWithT = new AtomicLong(0);
    private static final AtomicLong rayHitInstances = new AtomicLong(0);
    private static final AtomicLong hitInfoClones = new AtomicLong(0);
    private static final AtomicLong geometryDetailComputations = new AtomicLong(0);
    private static final AtomicLong instanceLevelTraversals = new AtomicLong(0);
    private static final AtomicLong instanceLevelNodeVisits = new AtomicLong(0);
    private static final AtomicLong instanceLevelEntryTests = new AtomicLong(0);
    private static final AtomicLong primitiveLevelTraversals = new AtomicLong(0);
    private static final AtomicLong primitiveLevelNodeVisits = new AtomicLong(0);
    private static final AtomicLong primitiveLevelPrimitiveTests = new AtomicLong(0);

    // -1: not decided yet (read the property), 0: disabled, 1: enabled
    private static int enabledState = -1;

    private RaytraceStatistics() {
    }

    public static boolean isEnabled() {
        if ( enabledState < 0 ) {
            enabledState = Boolean.getBoolean("vsdk.raytrace.stats") ? 1 : 0;
        }
        return enabledState == 1;
    }

    /**
    Overrides the system property; call it before tracing starts
    */
    public static void setEnabled(boolean enabled) {
        enabledState = enabled ? 1 : 0;
    }

    public static void reset() {
        primaryRays.set(0);
        shadowRays.set(0);
        reflectionRays.set(0);
        sceneTraversals.set(0);
        objectIntersectionTests.set(0);
        raysWithT.set(0);
        rayHitInstances.set(0);
        hitInfoClones.set(0);
        geometryDetailComputations.set(0);
        instanceLevelTraversals.set(0);
        instanceLevelNodeVisits.set(0);
        instanceLevelEntryTests.set(0);
        primitiveLevelTraversals.set(0);
        primitiveLevelNodeVisits.set(0);
        primitiveLevelPrimitiveTests.set(0);
    }

    public static void recordPrimaryRay() {
        if ( isEnabled() ) primaryRays.incrementAndGet();
    }

    public static void recordShadowRay() {
        if ( isEnabled() ) shadowRays.incrementAndGet();
    }

    public static void recordReflectionRay() {
        if ( isEnabled() ) reflectionRays.incrementAndGet();
    }

    public static void recordSceneTraversal() {
        if ( isEnabled() ) sceneTraversals.incrementAndGet();
    }

    public static void recordObjectIntersectionTest() {
        if ( isEnabled() ) objectIntersectionTests.incrementAndGet();
    }

    public static void recordRayWithT() {
        if ( isEnabled() ) raysWithT.incrementAndGet();
    }

    public static void recordRayHitInstance() {
        if ( isEnabled() ) rayHitInstances.incrementAndGet();
    }

    public static void recordHitInfoClone() {
        if ( isEnabled() ) hitInfoClones.incrementAndGet();
    }

    public static void recordGeometryDetailComputation() {
        if ( isEnabled() ) geometryDetailComputations.incrementAndGet();
    }

    /**
    One query of an instance level hierarchy
    */
    public static void recordInstanceLevelTraversal(long nodeVisits, long entryTests) {
        if ( !isEnabled() ) return;
        instanceLevelTraversals.incrementAndGet();
        if ( nodeVisits != 0 ) instanceLevelNodeVisits.addAndGet(nodeVisits);
        if ( entryTests != 0 ) instanceLevelEntryTests.addAndGet(entryTests);
    }

    /**
    One query of a primitive level hierarchy (or brute force loop)
    */
    public static void recordPrimitiveLevelTraversal(long nodeVisits, long primitiveTests) {
        if ( !isEnabled() ) return;
        primitiveLevelTraversals.incrementAndGet();
        if ( nodeVisits != 0 ) primitiveLevelNodeVisits.addAndGet(nodeVisits);
        if ( primitiveTests != 0 ) primitiveLevelPrimitiveTests.addAndGet(primitiveTests);
    }

    public static long getPrimaryRays() { return primaryRays.get(); }
    public static long getShadowRays() { return shadowRays.get(); }
    public static long getInstanceLevelTraversals() { return instanceLevelTraversals.get(); }
    public static long getInstanceLevelNodeVisits() { return instanceLevelNodeVisits.get(); }
    public static long getInstanceLevelEntryTests() { return instanceLevelEntryTests.get(); }
    public static long getPrimitiveLevelTraversals() { return primitiveLevelTraversals.get(); }
    public static long getPrimitiveLevelNodeVisits() { return primitiveLevelNodeVisits.get(); }
    public static long getPrimitiveLevelPrimitiveTests() { return primitiveLevelPrimitiveTests.get(); }

    public static void printSummary() {
        System.out.printf("Raytrace statistics (enabled=%s):\n", isEnabled() ? "true" : "false");
        System.out.printf("  Primary rays: %d\n", primaryRays.get());
        System.out.printf("  Shadow rays: %d\n", shadowRays.get());
        System.out.printf("  Reflection rays: %d\n", reflectionRays.get());
        System.out.printf("  Scene traversals: %d\n", sceneTraversals.get());
        System.out.printf("  Object intersection tests: %d\n", objectIntersectionTests.get());
        System.out.printf("  Rays with t: %d\n", raysWithT.get());
        System.out.printf("  RayHit instances: %d\n", rayHitInstances.get());
        System.out.printf("  Hit info clones: %d\n", hitInfoClones.get());
        System.out.printf("  Geometry detail computations: %d\n", geometryDetailComputations.get());
        System.out.printf("  Instance level: %d traversals, %d node visits, %d entry tests\n",
            instanceLevelTraversals.get(), instanceLevelNodeVisits.get(),
            instanceLevelEntryTests.get());
        System.out.printf("  Primitive level: %d traversals, %d node visits, %d primitive tests\n",
            primitiveLevelTraversals.get(), primitiveLevelNodeVisits.get(),
            primitiveLevelPrimitiveTests.get());
    }
}

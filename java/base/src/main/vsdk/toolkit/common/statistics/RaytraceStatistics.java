package vsdk.toolkit.common.statistics;

import java.util.concurrent.atomic.LongAdder;

/**
Counters of the CPU ray tracer, enabled with the system property
`vsdk.raytrace.stats` (read when the class is loaded) or with
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
    private static volatile boolean enabled = Boolean.getBoolean("vsdk.raytrace.stats");

    private static final LongAdder primaryRays = new LongAdder();
    private static final LongAdder shadowRays = new LongAdder();
    private static final LongAdder reflectionRays = new LongAdder();
    private static final LongAdder sceneTraversals = new LongAdder();
    private static final LongAdder objectIntersectionTests = new LongAdder();
    private static final LongAdder rayWithTCalls = new LongAdder();
    private static final LongAdder rayHitInstances = new LongAdder();
    private static final LongAdder hitInfoClones = new LongAdder();
    private static final LongAdder geometryDetailComputations = new LongAdder();
    private static final LongAdder instanceLevelTraversals = new LongAdder();
    private static final LongAdder instanceLevelNodeVisits = new LongAdder();
    private static final LongAdder instanceLevelEntryTests = new LongAdder();
    private static final LongAdder primitiveLevelTraversals = new LongAdder();
    private static final LongAdder primitiveLevelNodeVisits = new LongAdder();
    private static final LongAdder primitiveLevelPrimitiveTests = new LongAdder();

    private RaytraceStatistics() {
    }

    public static boolean isEnabled() {
        return enabled;
    }

    /** Overrides the system property; call it before tracing starts */
    public static void setEnabled(boolean enabled) {
        RaytraceStatistics.enabled = enabled;
    }

    public static void reset() {
        primaryRays.reset();
        shadowRays.reset();
        reflectionRays.reset();
        sceneTraversals.reset();
        objectIntersectionTests.reset();
        rayWithTCalls.reset();
        rayHitInstances.reset();
        hitInfoClones.reset();
        geometryDetailComputations.reset();
        instanceLevelTraversals.reset();
        instanceLevelNodeVisits.reset();
        instanceLevelEntryTests.reset();
        primitiveLevelTraversals.reset();
        primitiveLevelNodeVisits.reset();
        primitiveLevelPrimitiveTests.reset();
    }

    public static void recordPrimaryRay() {
        if (enabled) {
            primaryRays.increment();
        }
    }

    public static void recordShadowRay() {
        if (enabled) {
            shadowRays.increment();
        }
    }

    public static void recordReflectionRay() {
        if (enabled) {
            reflectionRays.increment();
        }
    }

    public static void recordSceneTraversal() {
        if (enabled) {
            sceneTraversals.increment();
        }
    }

    public static void recordObjectIntersectionTest() {
        if (enabled) {
            objectIntersectionTests.increment();
        }
    }

    public static void recordRayWithT() {
        if (enabled) {
            rayWithTCalls.increment();
        }
    }

    public static void recordRayHitInstance() {
        if (enabled) {
            rayHitInstances.increment();
        }
    }

    public static void recordHitInfoClone() {
        if (enabled) {
            hitInfoClones.increment();
        }
    }

    public static void recordGeometryDetailComputation() {
        if (enabled) {
            geometryDetailComputations.increment();
        }
    }

    /** One query of an instance level hierarchy */
    public static void recordInstanceLevelTraversal(long nodeVisits, long entryTests) {
        if (!enabled) {
            return;
        }
        instanceLevelTraversals.increment();
        if (nodeVisits != 0) {
            instanceLevelNodeVisits.add(nodeVisits);
        }
        if (entryTests != 0) {
            instanceLevelEntryTests.add(entryTests);
        }
    }

    /** One query of a primitive level hierarchy (or brute force loop) */
    public static void recordPrimitiveLevelTraversal(long nodeVisits, long primitiveTests) {
        if (!enabled) {
            return;
        }
        primitiveLevelTraversals.increment();
        if (nodeVisits != 0) {
            primitiveLevelNodeVisits.add(nodeVisits);
        }
        if (primitiveTests != 0) {
            primitiveLevelPrimitiveTests.add(primitiveTests);
        }
    }

    public static long getPrimaryRays() {
        return primaryRays.sum();
    }

    public static long getShadowRays() {
        return shadowRays.sum();
    }

    public static long getInstanceLevelTraversals() {
        return instanceLevelTraversals.sum();
    }

    public static long getInstanceLevelNodeVisits() {
        return instanceLevelNodeVisits.sum();
    }

    public static long getInstanceLevelEntryTests() {
        return instanceLevelEntryTests.sum();
    }

    public static long getPrimitiveLevelTraversals() {
        return primitiveLevelTraversals.sum();
    }

    public static long getPrimitiveLevelNodeVisits() {
        return primitiveLevelNodeVisits.sum();
    }

    public static long getPrimitiveLevelPrimitiveTests() {
        return primitiveLevelPrimitiveTests.sum();
    }

    public static void printSummary() {
        if (!enabled) {
            return;
        }

        long primaryRaysSnapshot = primaryRays.sum();
        long shadowRaysSnapshot = shadowRays.sum();
        long reflectionRaysSnapshot = reflectionRays.sum();
        long sceneTraversalsSnapshot = sceneTraversals.sum();
        long objectIntersectionTestsSnapshot = objectIntersectionTests.sum();
        long rayWithTCallsSnapshot = rayWithTCalls.sum();
        long rayHitInstancesSnapshot = rayHitInstances.sum();
        long hitInfoClonesSnapshot = hitInfoClones.sum();
        long geometryDetailComputationsSnapshot = geometryDetailComputations.sum();
        long totalRays =
            primaryRaysSnapshot + shadowRaysSnapshot + reflectionRaysSnapshot;

        System.out.println("Ray statistics:");
        System.out.println("  Primary rays: " + primaryRaysSnapshot);
        System.out.println("  Shadow rays: " + shadowRaysSnapshot);
        System.out.println("  Reflection rays: " + reflectionRaysSnapshot);
        System.out.println("  Total rays cast: " + totalRays);
        System.out.println("  Scene traversals: " + sceneTraversalsSnapshot);
        System.out.println("  Object intersection tests: " + objectIntersectionTestsSnapshot);
        System.out.println("  Ray.withT calls: " + rayWithTCallsSnapshot);
        System.out.println("  RayHit instances: " + rayHitInstancesSnapshot);
        System.out.println("  Hit info clones: " + hitInfoClonesSnapshot);
        System.out.println("  Geometry detail computations: " + geometryDetailComputationsSnapshot);
        if (sceneTraversalsSnapshot > 0) {
            System.out.println(
                "  Avg. object tests / traversal: " +
                ((double) objectIntersectionTestsSnapshot / (double) sceneTraversalsSnapshot)
            );
        }
        System.out.println("  Instance level: " + instanceLevelTraversals.sum() +
            " traversals, " + instanceLevelNodeVisits.sum() + " node visits, " +
            instanceLevelEntryTests.sum() + " entry tests");
        System.out.println("  Primitive level: " + primitiveLevelTraversals.sum() +
            " traversals, " + primitiveLevelNodeVisits.sum() + " node visits, " +
            primitiveLevelPrimitiveTests.sum() + " primitive tests");
    }
}

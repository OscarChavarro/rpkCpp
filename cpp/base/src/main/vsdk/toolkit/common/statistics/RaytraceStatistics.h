#ifndef __RAYTRACE_STATISTICS__
#define __RAYTRACE_STATISTICS__

/**
Counters of the CPU ray tracer, enabled with the system property
`vsdk.raytrace.stats` (see `java::System::setProperty`, read the first time
the statistics are used) or with `setEnabled`. Disabled, every `record`
call is one test of a flag.

Acceleration structure counters follow the two levels of
`doc/flatBinaryTreeContract.md`, with the same meaning as the GPU counters
of the Vulkan 1.4 compute path (`VulkanComputeTraversalCounters`):

- instance level: hierarchies whose leaves hold meshes or transformed
  instances (`BinnedSahHierarchy::traverseClosest`, used by
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
class RaytraceStatistics {
public:
    static bool isEnabled();
    /** Overrides the system property; call it before tracing starts */
    static void setEnabled(bool enabled);
    static void reset();

    static void recordPrimaryRay();
    static void recordShadowRay();
    static void recordReflectionRay();
    static void recordSceneTraversal();
    static void recordObjectIntersectionTest();
    static void recordRayWithT();
    static void recordRayHitInstance();
    static void recordHitInfoClone();
    static void recordGeometryDetailComputation();

    /** One query of an instance level hierarchy */
    static void recordInstanceLevelTraversal(long int nodeVisits, long int entryTests);
    /** One query of a primitive level hierarchy (or brute force loop) */
    static void recordPrimitiveLevelTraversal(long int nodeVisits, long int primitiveTests);

    static long long getPrimaryRays();
    static long long getShadowRays();
    static long long getInstanceLevelTraversals();
    static long long getInstanceLevelNodeVisits();
    static long long getInstanceLevelEntryTests();
    static long long getPrimitiveLevelTraversals();
    static long long getPrimitiveLevelNodeVisits();
    static long long getPrimitiveLevelPrimitiveTests();

    static void printSummary();

private:
    RaytraceStatistics() {}
};

#endif

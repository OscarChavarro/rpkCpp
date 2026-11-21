type StatisticsFlags = { readonly raytrace?: boolean; readonly polyhedral?: boolean };
const statisticsFlags = (globalThis as { __vitralStatistics?: StatisticsFlags }).__vitralStatistics;

/**
Counters of the CPU ray tracer, enabled with the browser-safe counterpart of
the system property `vsdk.raytrace.stats` (`globalThis.__vitralStatistics.raytrace`,
read the first time the statistics are used) or with `setEnabled`. Disabled,
every `record` call is one test of a flag.

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

Traversals add their counts once per query, not once per node. Counters
live in the current JavaScript realm: work done inside workers is not added
to the counters of the main thread.
*/
export class RaytraceStatistics {
    // -1: not decided yet (read the flag), 0: disabled, 1: enabled
    private static enabledState = -1;
    private static primaryRays = 0n;
    private static shadowRays = 0n;
    private static reflectionRays = 0n;
    private static sceneTraversals = 0n;
    private static objectIntersectionTests = 0n;
    private static rayWithTCalls = 0n;
    private static rayHitInstances = 0n;
    private static hitInfoClones = 0n;
    private static geometryDetailComputations = 0n;
    private static instanceLevelTraversals = 0n;
    private static instanceLevelNodeVisits = 0n;
    private static instanceLevelEntryTests = 0n;
    private static primitiveLevelTraversals = 0n;
    private static primitiveLevelNodeVisits = 0n;
    private static primitiveLevelPrimitiveTests = 0n;

    private constructor() {}

    public static isEnabled(): boolean {
        if (this.enabledState < 0) {
            this.enabledState = statisticsFlags?.raytrace === true ? 1 : 0;
        }
        return this.enabledState === 1;
    }

    /** Overrides the global flag; call it before tracing starts */
    public static setEnabled(enabled: boolean): void {
        this.enabledState = enabled ? 1 : 0;
    }

    public static reset(): void {
        this.primaryRays = 0n;
        this.shadowRays = 0n;
        this.reflectionRays = 0n;
        this.sceneTraversals = 0n;
        this.objectIntersectionTests = 0n;
        this.rayWithTCalls = 0n;
        this.rayHitInstances = 0n;
        this.hitInfoClones = 0n;
        this.geometryDetailComputations = 0n;
        this.instanceLevelTraversals = 0n;
        this.instanceLevelNodeVisits = 0n;
        this.instanceLevelEntryTests = 0n;
        this.primitiveLevelTraversals = 0n;
        this.primitiveLevelNodeVisits = 0n;
        this.primitiveLevelPrimitiveTests = 0n;
    }

    public static recordPrimaryRay(): void {
        if (this.isEnabled()) this.primaryRays++;
    }
    public static recordShadowRay(): void {
        if (this.isEnabled()) this.shadowRays++;
    }
    public static recordReflectionRay(): void {
        if (this.isEnabled()) this.reflectionRays++;
    }
    public static recordSceneTraversal(): void {
        if (this.isEnabled()) this.sceneTraversals++;
    }
    public static recordObjectIntersectionTest(): void {
        if (this.isEnabled()) this.objectIntersectionTests++;
    }
    public static recordRayWithT(): void {
        if (this.isEnabled()) this.rayWithTCalls++;
    }
    public static recordRayHitInstance(): void {
        if (this.isEnabled()) this.rayHitInstances++;
    }
    public static recordHitInfoClone(): void {
        if (this.isEnabled()) this.hitInfoClones++;
    }
    public static recordGeometryDetailComputation(): void {
        if (this.isEnabled()) this.geometryDetailComputations++;
    }

    /** One query of an instance level hierarchy */
    public static recordInstanceLevelTraversal(nodeVisits: number, entryTests: number): void {
        if (!this.isEnabled()) return;
        this.instanceLevelTraversals++;
        if (nodeVisits !== 0) this.instanceLevelNodeVisits += BigInt(nodeVisits);
        if (entryTests !== 0) this.instanceLevelEntryTests += BigInt(entryTests);
    }

    /** One query of a primitive level hierarchy (or brute force loop) */
    public static recordPrimitiveLevelTraversal(nodeVisits: number, primitiveTests: number): void {
        if (!this.isEnabled()) return;
        this.primitiveLevelTraversals++;
        if (nodeVisits !== 0) this.primitiveLevelNodeVisits += BigInt(nodeVisits);
        if (primitiveTests !== 0) this.primitiveLevelPrimitiveTests += BigInt(primitiveTests);
    }

    public static getPrimaryRays(): number {
        return Number(this.primaryRays);
    }
    public static getShadowRays(): number {
        return Number(this.shadowRays);
    }
    public static getInstanceLevelTraversals(): number {
        return Number(this.instanceLevelTraversals);
    }
    public static getInstanceLevelNodeVisits(): number {
        return Number(this.instanceLevelNodeVisits);
    }
    public static getInstanceLevelEntryTests(): number {
        return Number(this.instanceLevelEntryTests);
    }
    public static getPrimitiveLevelTraversals(): number {
        return Number(this.primitiveLevelTraversals);
    }
    public static getPrimitiveLevelNodeVisits(): number {
        return Number(this.primitiveLevelNodeVisits);
    }
    public static getPrimitiveLevelPrimitiveTests(): number {
        return Number(this.primitiveLevelPrimitiveTests);
    }

    public static printSummary(): void {
        if (!this.isEnabled()) return;
        const total = this.primaryRays + this.shadowRays + this.reflectionRays;
        console.log(
            `Ray statistics:\n  Primary rays: ${this.primaryRays}\n  Shadow rays: ${this.shadowRays}\n  Reflection rays: ${this.reflectionRays}\n  Total rays cast: ${total}\n  Scene traversals: ${this.sceneTraversals}\n  Object intersection tests: ${this.objectIntersectionTests}\n  Ray.withT calls: ${this.rayWithTCalls}\n  RayHit instances: ${this.rayHitInstances}\n  Hit info clones: ${this.hitInfoClones}\n  Geometry detail computations: ${this.geometryDetailComputations}${this.sceneTraversals > 0n ? `\n  Avg. object tests / traversal: ${Number(this.objectIntersectionTests) / Number(this.sceneTraversals)}` : ""}` +
                `\n  Instance level: ${this.instanceLevelTraversals} traversals, ${this.instanceLevelNodeVisits} node visits, ${this.instanceLevelEntryTests} entry tests` +
                `\n  Primitive level: ${this.primitiveLevelTraversals} traversals, ${this.primitiveLevelNodeVisits} node visits, ${this.primitiveLevelPrimitiveTests} primitive tests`,
        );
    }
}

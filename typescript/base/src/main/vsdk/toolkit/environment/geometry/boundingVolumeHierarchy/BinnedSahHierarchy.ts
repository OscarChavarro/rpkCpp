//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =

import { ArrayList } from "../../../../../java/util/ArrayList.js";
import { RaytraceStatistics } from "../../../common/statistics/RaytraceStatistics.js";
import type { Ray } from "../element/Ray.js";
import { BinnedSahHierarchyNode } from "./BinnedSahHierarchyNode.js";
import type { BinnedSahLeafVisitor } from "./BinnedSahLeafVisitor.js";
import type { BinnedSahStoppingLeafVisitor } from "./BinnedSahStoppingLeafVisitor.js";
import { _BinnedSahBox } from "./_BinnedSahBox.js";
import { _BinnedSahBuildTask } from "./_BinnedSahBuildTask.js";

/**
Binary bounding volume hierarchy over axis aligned primitive boxes, built
top-down with the binned surface area heuristic of [WALD2007].3. It is the
single builder of vitral for binary hierarchies: it serves the triangles of
a mesh (`BinnedSahTriangleHierarchy`, bottom level), the meshes of a
`TriangleMeshGroup` and the transformed instances of a
`BinnedSahInstanceHierarchy` (top level), and the Vulkan 1.4 raytracer
uploads its result to the GPU for the compute path without
`VK_KHR_acceleration_structure`. The output follows the flat binary tree
contract of `doc/flatBinaryTreeContract.md`. Its traversal is the one of
`casTraverseBlas` in `computeAccelerationStructure.glsl`: a stack, nearest
child first, nodes farther than the closest hit skipped.

Build, following [WALD2007].3: primitives are binned by the centroid of
their box into `BIN_COUNT` bins that uniformly subdivide the centroid bounds
([WALD2007].3.1, .3.3); each bin keeps its count and the full 3D box of its
members, grown from a negative box ([WALD2007].3.4); the cost
`A_L N_L + A_R N_R` of the `BIN_COUNT - 1` partitions is evaluated with one
sweep from each side ([WALD2007].3.1, .3.5); the build rearranges one
primitive id array, with a temporary array ([WALD2007].3.6).

Deviations from [WALD2007]:
- the three axes are binned, not only the widest one (the alternative
  mentioned in [WALD2007].3.1);
- 12 bins (16 recommended, 4 already reasonable, [WALD2007].3.7);
- bin index clamped to `[0, BIN_COUNT - 1]` instead of the `K(1 - epsilon)`
  factor of [WALD2007].3.3;
- termination ([WALD2007].3.8): a node with all its centroids at one point
  is split in halves instead of becoming a leaf, a split more expensive
  than the leaf is still taken while the node holds more than twice
  `maximumLeafSize` primitives, and the depth is capped by
  `MAXIMUM_DEPTH` (bounded traversal stack);
- single threaded and deterministic: the parallel build of [WALD2007].4 is
  not used, so it runs on every vitral platform.
*/
export class BinnedSahHierarchy {
    public static readonly BIN_COUNT = 12;
    public static readonly MAXIMUM_DEPTH = 30;
    /** Leaf size of `build` when none is given (bottom level) */
    public static readonly DEFAULT_MAXIMUM_LEAF_SIZE = 4;
    /** Traversal stack; it never holds more than `MAXIMUM_DEPTH + 1`
    entries */
    public static readonly STACK_SIZE = 64;

    private readonly nodes: ArrayList<BinnedSahHierarchyNode> = new ArrayList<BinnedSahHierarchyNode>();
    private readonly primitiveOrder: ArrayList<number> = new ArrayList<number>();

    // Typed-array copy of `nodes` and `primitiveOrder` read by the traversal
    // (TypeScript only: object nodes behind ArrayList.get cost a third of the
    // CPU time of a Galerkin run). Refreshed by `pack` after every build and
    // whenever the node count no longer matches.
    /** Per node: minimum x, y, z, maximum x, y, z */
    private packedBounds: Float64Array = new Float64Array(0);
    /** Per node: leftOrFirst, count */
    private packedLinks: Int32Array = new Int32Array(0);
    private packedOrder: Int32Array = new Int32Array(0);
    /** Traversal stacks reused across queries, one pair per nesting level
    (a visitor may start another traversal of this hierarchy) */
    private readonly nodeStacks: Int32Array[] = [];
    private readonly distanceStacks: Float64Array[] = [];
    private traversalDepth = 0;

    public constructor();
    /** Deep copy (cpp copies the hierarchy by value) */
    public constructor(other: BinnedSahHierarchy);
    public constructor(other?: BinnedSahHierarchy) {
        if (other === undefined) {
            return;
        }
        for (let i = 0; i < other.nodes.size(); i++) {
            this.nodes.add(new BinnedSahHierarchyNode(other.nodes.get(i)));
        }
        this.primitiveOrder.addAll(other.primitiveOrder);
        this.pack();
    }

    /** Copies `nodes` and `primitiveOrder` into the typed arrays of the
    traversal */
    private pack(): void {
        const nodeCount = this.nodes.size();
        this.packedBounds = new Float64Array(6 * nodeCount);
        this.packedLinks = new Int32Array(2 * nodeCount);
        for (let i = 0; i < nodeCount; i++) {
            const node = this.nodes.get(i);
            for (let a = 0; a < 3; a++) {
                this.packedBounds[6 * i + a] = node.minimum[a]!;
                this.packedBounds[6 * i + 3 + a] = node.maximum[a]!;
            }
            this.packedLinks[2 * i] = node.leftOrFirst;
            this.packedLinks[2 * i + 1] = node.count;
        }
        const primitiveCount = this.primitiveOrder.size();
        this.packedOrder = new Int32Array(primitiveCount);
        for (let i = 0; i < primitiveCount; i++) {
            this.packedOrder[i] = this.primitiveOrder.get(i);
        }
    }

    private static setNodeBounds(node: BinnedSahHierarchyNode, box: _BinnedSahBox): void {
        for (let a = 0; a < 3; a++) {
            node.minimum[a] = box.minimum[a]!;
            node.maximum[a] = box.maximum[a]!;
        }
    }

    private static binOf(centroid: number, lo: number, scale: number): number {
        // [WALD2007].3.3 Triangle-to-bin projection binID = k1 (c - k0); the
        // clamp replaces the (1 - epsilon) factor for centroids on the upper
        // bound of the centroid box.
        let bin = Math.trunc((centroid - lo) * scale);
        if (bin < 0) {
            bin = 0;
        }
        if (bin > BinnedSahHierarchy.BIN_COUNT - 1) {
            bin = BinnedSahHierarchy.BIN_COUNT - 1;
        }
        return bin;
    }

    /** `outAxis`, `outBin` and `outCost` are one-element output holders. */
    private static findSplit(
        boxes: ArrayList<_BinnedSahBox>,
        order: ArrayList<number>,
        first: number,
        count: number,
        centroidBox: _BinnedSahBox,
        outAxis: number[],
        outBin: number[],
        outCost: number[],
    ): boolean {
        const BINS = BinnedSahHierarchy.BIN_COUNT;
        let found = false;
        for (let axis = 0; axis < 3; axis++) {
            const extent = centroidBox.extent(axis);
            if (!(extent > 0.0)) {
                continue;
            }
            // [WALD2007].3.1 Bins of equal width over the centroid bounds; all
            // three axes are tried, [WALD2007] bins only the widest one.
            const lo = centroidBox.minimum[axis]!;
            const scale = BINS / extent;

            // [WALD2007].3.4 Bin updates: count and full 3D bounds per bin,
            // starting from a negative box.
            const binBoxes: _BinnedSahBox[] = new Array<_BinnedSahBox>(BinnedSahHierarchy.BIN_COUNT);
            const binCounts: number[] = new Array<number>(BinnedSahHierarchy.BIN_COUNT);
            for (let b = 0; b < BINS; b++) {
                binBoxes[b] = new _BinnedSahBox();
                binCounts[b] = 0;
            }
            for (let i = first; i < first + count; i++) {
                const box = boxes.get(order.get(i));
                const b = BinnedSahHierarchy.binOf(box.centroid(axis), lo, scale);
                binCounts[b]!++;
                binBoxes[b]!.enclose(box);
            }

            // [WALD2007].3.5 Plane evaluations: sweep from the right, then from
            // the left; the cost of a split after bin `b` is
            // countLeft * areaLeft + countRight * areaRight ([WALD2007].3.1).
            // Partitions with an empty side are rejected.
            const rightArea: number[] = new Array<number>(BinnedSahHierarchy.BIN_COUNT).fill(0);
            const rightCount: number[] = new Array<number>(BinnedSahHierarchy.BIN_COUNT).fill(0);
            let accumulated = new _BinnedSahBox();
            let accumulatedCount = 0;
            for (let b = BINS - 1; b > 0; b--) {
                accumulated.enclose(binBoxes[b]!);
                accumulatedCount += binCounts[b]!;
                rightArea[b] = accumulated.halfArea();
                rightCount[b] = accumulatedCount;
            }
            accumulated = new _BinnedSahBox();
            accumulatedCount = 0;
            for (let b = 0; b < BINS - 1; b++) {
                accumulated.enclose(binBoxes[b]!);
                accumulatedCount += binCounts[b]!;
                if (accumulatedCount === 0 || rightCount[b + 1] === 0) {
                    continue;
                }
                const cost = accumulatedCount * accumulated.halfArea() + rightCount[b + 1]! * rightArea[b + 1]!;
                if (!found || cost < outCost[0]!) {
                    found = true;
                    outCost[0] = cost;
                    outAxis[0] = axis;
                    outBin[0] = b;
                }
            }
        }
        return found;
    }

    private static emptyNode(): BinnedSahHierarchyNode {
        const node = new BinnedSahHierarchyNode();
        BinnedSahHierarchy.setNodeBounds(node, new _BinnedSahBox());
        node.leftOrFirst = 0;
        node.count = 0;
        return node;
    }

    /**
    @param boxValues six values per primitive: lo x, y, z, hi x, y, z.
    Callers pad the boxes so rounding never hides a hit. An empty box
    (lo > hi) is allowed: its primitive is never reached.
    @param maximumLeafSize leaves are split while they hold more primitives
    than this and a split is cheaper (SAH), or while they hold more than
    twice this; values below 1 count as 1. Top levels over instances use 1,
    as a hardware TLAS.
    */
    public build(
        boxValues: ArrayList<number>,
        maximumLeafSize: number = BinnedSahHierarchy.DEFAULT_MAXIMUM_LEAF_SIZE,
    ): void {
        if (maximumLeafSize < 1) {
            maximumLeafSize = 1;
        }
        const primitiveCount = Math.trunc(boxValues.size() / 6);

        // [WALD2007].3.2 Initial setup: the box of every primitive, given by
        // the caller; the build only looks at these boxes.
        const boxes = new ArrayList<_BinnedSahBox>(primitiveCount > 0 ? primitiveCount : 1);
        for (let i = 0; i < primitiveCount; i++) {
            const box = new _BinnedSahBox();
            for (let a = 0; a < 3; a++) {
                box.minimum[a] = boxValues.get(6 * i + a);
                box.maximum[a] = boxValues.get(6 * i + 3 + a);
            }
            boxes.add(box);
        }

        // [WALD2007].3.6 Nodes and primitive ids in two arrays, at most
        // 2N - 1 nodes, ids initialized to 0, 1, 2, ...
        this.nodes.clear();
        this.primitiveOrder.clear();
        for (let i = 0; i < primitiveCount; i++) {
            this.primitiveOrder.add(i);
        }

        this.nodes.add(BinnedSahHierarchy.emptyNode());
        if (primitiveCount === 0) {
            // One empty leaf: its negative box never passes the slab test.
            this.pack();
            return;
        }

        const stack = new ArrayList<_BinnedSahBuildTask>();
        let task = new _BinnedSahBuildTask();
        task.node = 0;
        task.first = 0;
        task.count = primitiveCount;
        task.depth = 1;
        stack.add(task);

        const scratch = new ArrayList<number>(primitiveCount);
        const axisHolder: number[] = [0];
        const splitBinHolder: number[] = [0];
        const splitCostHolder: number[] = [0];
        while (stack.size() > 0) {
            task = stack.remove(stack.size() - 1);

            // [WALD2007].3.2 Node bounds and centroid bounds of the range
            // (recomputed per node instead of tracked while partitioning).
            const bounds = new _BinnedSahBox();
            const centroidBox = new _BinnedSahBox();
            const c: number[] = [0, 0, 0];
            for (let i = task.first; i < task.first + task.count; i++) {
                const box = boxes.get(this.primitiveOrder.get(i));
                bounds.enclose(box);
                for (let a = 0; a < 3; a++) {
                    c[a] = box.centroid(a);
                }
                centroidBox.enclosePoint(c);
            }
            const taskNode = this.nodes.get(task.node);
            BinnedSahHierarchy.setNodeBounds(taskNode, bounds);
            taskNode.leftOrFirst = task.first;
            taskNode.count = task.count;

            // [WALD2007].3.8 Termination a): few primitives. The depth cap is
            // not in [WALD2007]: it bounds the traversal stack.
            if (task.count <= maximumLeafSize || task.depth >= BinnedSahHierarchy.MAXIMUM_DEPTH) {
                continue;
            }

            axisHolder[0] = 0;
            splitBinHolder[0] = 0;
            splitCostHolder[0] = 0.0;
            let leftCount = 0;
            if (
                BinnedSahHierarchy.findSplit(
                    boxes,
                    this.primitiveOrder,
                    task.first,
                    task.count,
                    centroidBox,
                    axisHolder,
                    splitBinHolder,
                    splitCostHolder,
                )
            ) {
                const axis = axisHolder[0]!;
                const splitBin = splitBinHolder[0]!;
                const splitCost = splitCostHolder[0]!;
                const leafCost = task.count * bounds.halfArea();
                // [WALD2007].3.8 Termination c): the split costs more than the
                // leaf (only for nodes small enough to stay a leaf).
                if (splitCost >= leafCost && task.count <= 2 * maximumLeafSize) {
                    continue;
                }
                // [WALD2007].3.6 ID list partitioning with a temporary array;
                // stable: members of bins [0, splitBin] first.
                const lo = centroidBox.minimum[axis]!;
                const scale = BinnedSahHierarchy.BIN_COUNT / centroidBox.extent(axis);
                scratch.clear();
                for (let i = task.first; i < task.first + task.count; i++) {
                    const p = this.primitiveOrder.get(i);
                    if (BinnedSahHierarchy.binOf(boxes.get(p).centroid(axis), lo, scale) <= splitBin) {
                        this.primitiveOrder.set(task.first + leftCount, p);
                        leftCount++;
                    } else {
                        scratch.add(p);
                    }
                }
                for (let i = 0; i < scratch.size(); i++) {
                    this.primitiveOrder.set(task.first + leftCount + i, scratch.get(i));
                }
            } else {
                // Every centroid is the same point: [WALD2007].3.8 b) would make
                // a leaf; the range is split in halves so the leaf size stays
                // bounded.
                leftCount = Math.trunc(task.count / 2);
            }

            const left = this.nodes.size();
            this.nodes.add(BinnedSahHierarchy.emptyNode());
            this.nodes.add(BinnedSahHierarchy.emptyNode());
            taskNode.leftOrFirst = left;
            taskNode.count = 0;

            // Right first, so the left subtree is flattened first (depth first).
            const rightTask = new _BinnedSahBuildTask();
            rightTask.node = left + 1;
            rightTask.first = task.first + leftCount;
            rightTask.count = task.count - leftCount;
            rightTask.depth = task.depth + 1;
            stack.add(rightTask);
            const leftTask = new _BinnedSahBuildTask();
            leftTask.node = left;
            leftTask.first = task.first;
            leftTask.count = leftCount;
            leftTask.depth = task.depth + 1;
            stack.add(leftTask);
        }
        this.pack();
    }

    /**
    The traversal loop shared by traverseClosest and traverse, on a non empty
    hierarchy: a stack, nearest child first, nodes farther than the closest hit
    skipped. Counts node visits and leaf entries into the given holders;
    returns false on stack overflow. The visitor ends the traversal by
    returning true.
    */
    private traverseNodes(
        ray: Ray,
        visit: (primitive: number, inOutBestT: number[]) => boolean,
        inOutBestT: number[],
        nodeVisits: number[],
        entryTests: number[],
    ): boolean {
        if (
            this.packedLinks.length !== 2 * this.nodes.size() ||
            this.packedOrder.length !== this.primitiveOrder.size()
        ) {
            this.pack();
        }
        const bounds = this.packedBounds;
        const links = this.packedLinks;
        const order = this.packedOrder;

        // Slab entry parameters (RaySlab), kept in locals
        const o = ray.getOrigin();
        const d = ray.getDirection();
        const ox = o.x();
        const oy = o.y();
        const oz = o.z();
        const dx = d.x();
        const dy = d.y();
        const dz = d.z();
        const px = dx === 0.0;
        const py = dy === 0.0;
        const pz = dz === 0.0;
        const ix = px ? 0.0 : 1.0 / dx;
        const iy = py ? 0.0 : 1.0 / dy;
        const iz = pz ? 0.0 : 1.0 / dz;
        // Entry parameter of the ray in the box of node n clipped to
        // [0, tMax], or -1 on a miss or an empty box (RaySlab.entry)
        const entry = (n: number, tMax: number): number => {
            const b = 6 * n;
            let tNear = 0.0;
            let tFar = tMax;
            let lo = bounds[b]!;
            let hi = bounds[b + 3]!;
            if (lo > hi) {
                return -1.0;
            }
            if (px) {
                if (ox < lo || ox > hi) {
                    return -1.0;
                }
            } else {
                let t0 = (lo - ox) * ix;
                let t1 = (hi - ox) * ix;
                if (t0 > t1) {
                    const swap = t0;
                    t0 = t1;
                    t1 = swap;
                }
                tNear = t0 > tNear ? t0 : tNear;
                tFar = t1 < tFar ? t1 : tFar;
                if (tNear > tFar) {
                    return -1.0;
                }
            }
            lo = bounds[b + 1]!;
            hi = bounds[b + 4]!;
            if (lo > hi) {
                return -1.0;
            }
            if (py) {
                if (oy < lo || oy > hi) {
                    return -1.0;
                }
            } else {
                let t0 = (lo - oy) * iy;
                let t1 = (hi - oy) * iy;
                if (t0 > t1) {
                    const swap = t0;
                    t0 = t1;
                    t1 = swap;
                }
                tNear = t0 > tNear ? t0 : tNear;
                tFar = t1 < tFar ? t1 : tFar;
                if (tNear > tFar) {
                    return -1.0;
                }
            }
            lo = bounds[b + 2]!;
            hi = bounds[b + 5]!;
            if (lo > hi) {
                return -1.0;
            }
            if (pz) {
                if (oz < lo || oz > hi) {
                    return -1.0;
                }
            } else {
                let t0 = (lo - oz) * iz;
                let t1 = (hi - oz) * iz;
                if (t0 > t1) {
                    const swap = t0;
                    t0 = t1;
                    t1 = swap;
                }
                tNear = t0 > tNear ? t0 : tNear;
                tFar = t1 < tFar ? t1 : tFar;
                if (tNear > tFar) {
                    return -1.0;
                }
            }
            return tNear;
        };

        const rootDistance = entry(0, inOutBestT[0]!);
        if (rootDistance < 0.0) {
            return true;
        }
        const level = this.traversalDepth;
        if (level === this.nodeStacks.length) {
            this.nodeStacks.push(new Int32Array(BinnedSahHierarchy.STACK_SIZE));
            this.distanceStacks.push(new Float64Array(BinnedSahHierarchy.STACK_SIZE));
        }
        const nodeStack = this.nodeStacks[level]!;
        const distanceStack = this.distanceStacks[level]!;
        this.traversalDepth++;
        try {
            let visits = 0;
            let tests = 0;
            let sp = 0;
            nodeStack[sp] = 0;
            distanceStack[sp] = rootDistance;
            sp++;
            while (sp > 0) {
                sp--;
                if (distanceStack[sp]! > inOutBestT[0]!) {
                    continue;
                }
                const node = nodeStack[sp]!;
                visits++;
                const first = links[2 * node]!;
                const count = links[2 * node + 1]!;
                if (count > 0) {
                    for (let i = first; i < first + count; i++) {
                        if (visit(order[i]!, inOutBestT)) {
                            nodeVisits[0] = nodeVisits[0]! + visits;
                            entryTests[0] = entryTests[0]! + tests + i - first + 1;
                            return true;
                        }
                    }
                    tests += count;
                    continue;
                }
                let left = first;
                let right = left + 1;
                let dl = entry(left, inOutBestT[0]!);
                let dr = entry(right, inOutBestT[0]!);
                // Push the farther child first, so the nearer one is visited first.
                if (dl >= 0.0 && dr >= 0.0 && dl < dr) {
                    const swapNode = left;
                    left = right;
                    right = swapNode;
                    const swapDistance = dl;
                    dl = dr;
                    dr = swapDistance;
                }
                if (sp + 2 > BinnedSahHierarchy.STACK_SIZE) {
                    nodeVisits[0] = nodeVisits[0]! + visits;
                    entryTests[0] = entryTests[0]! + tests;
                    return false;
                }
                if (dl >= 0.0) {
                    nodeStack[sp] = left;
                    distanceStack[sp] = dl;
                    sp++;
                }
                if (dr >= 0.0) {
                    nodeStack[sp] = right;
                    distanceStack[sp] = dr;
                    sp++;
                }
            }
            nodeVisits[0] = nodeVisits[0]! + visits;
            entryTests[0] = entryTests[0]! + tests;
        } finally {
            this.traversalDepth--;
        }
        return true;
    }

    /**
    Closest hit traversal: calls the visitor for the primitives of every
    leaf the ray reaches before `inOutBestT[0]`, nearest nodes first. A node
    whose entry equals `inOutBestT[0]` is still visited, so a visitor
    resolving ties by primitive index gets the same answer as a linear
    loop. Returns false when the stack would overflow (not reachable with
    `MAXIMUM_DEPTH < STACK_SIZE`); the caller then uses its linear loop.
    Node visits and leaf entries are reported to `RaytraceStatistics` as
    instance level work (`recordInstanceLevelTraversal`).
    @param inOutBestT one-element input/output holder
    */
    public traverseClosest(ray: Ray, visitor: BinnedSahLeafVisitor, inOutBestT: number[]): boolean {
        if (this.nodes.size() === 0 || this.primitiveOrder.size() === 0) {
            return true;
        }
        // Counted locally, reported once: no shared write per node.
        const nodeVisits = [0];
        const entryTests = [0];
        const completed = this.traverseNodes(
            ray,
            (primitive: number, bestT: number[]): boolean => {
                visitor.visit(primitive, bestT);
                return false;
            },
            inOutBestT,
            nodeVisits,
            entryTests,
        );
        RaytraceStatistics.recordInstanceLevelTraversal(nodeVisits[0]!, entryTests[0]!);
        return completed;
    }

    /**
    The traversal of `traverseClosest` for callers that keep their own
    statistics and may stop early: the visitor ends the traversal by
    returning true, and node visits and leaf entries are added to the given
    counters instead of being reported to `RaytraceStatistics` (nothing
    static is written). Node entry is clipped at parameter zero: a caller
    with a minimum parameter applies it in its visitor. Returns false when
    the stack would overflow, as `traverseClosest`.
    @param inOutBestT, nodeVisits, entryTests one-element input/output holders
    */
    public traverse(
        ray: Ray,
        visitor: BinnedSahStoppingLeafVisitor,
        inOutBestT: number[],
        nodeVisits: number[],
        entryTests: number[],
    ): boolean {
        if (this.nodes.size() === 0 || this.primitiveOrder.size() === 0) {
            return true;
        }
        return this.traverseNodes(
            ray,
            (primitive: number, bestT: number[]): boolean => visitor.visit(primitive, bestT),
            inOutBestT,
            nodeVisits,
            entryTests,
        );
    }

    public getNodes(): ArrayList<BinnedSahHierarchyNode> {
        return this.nodes;
    }

    /** Primitive ids in leaf order */
    public getPrimitiveOrder(): ArrayList<number> {
        return this.primitiveOrder;
    }

    public getNodeCount(): number {
        return this.nodes.size();
    }

    /** Depth of the hierarchy (a single leaf has depth 1) */
    public getDepth(): number {
        if (this.nodes.size() === 0) {
            return 0;
        }
        const pendingNodes = new ArrayList<number>();
        const pendingDepths = new ArrayList<number>();
        pendingNodes.add(0);
        pendingDepths.add(1);
        let maximum = 0;
        while (pendingNodes.size() > 0) {
            const last = pendingNodes.size() - 1;
            const node = pendingNodes.get(last);
            const d = pendingDepths.get(last);
            pendingNodes.remove(last);
            pendingDepths.remove(last);
            if (d > maximum) {
                maximum = d;
            }
            // Only the root of an empty hierarchy is a leaf without primitives.
            if (this.nodes.get(node).count === 0 && this.nodes.size() > 1) {
                pendingNodes.add(this.nodes.get(node).leftOrFirst);
                pendingDepths.add(d + 1);
                pendingNodes.add(this.nodes.get(node).leftOrFirst + 1);
                pendingDepths.add(d + 1);
            }
        }
        return maximum;
    }
}

export namespace BinnedSahHierarchy {
    /** Slab entry parameters of a ray, shared by the traversals */
    export class RaySlab {
        public readonly origin: number[];
        public readonly inverse: number[];
        public readonly parallel: boolean[];

        public constructor(ray: Ray) {
            const o = ray.getOrigin();
            const d = ray.getDirection();
            this.origin = [o.x(), o.y(), o.z()];
            const direction = [d.x(), d.y(), d.z()];
            this.inverse = [0, 0, 0];
            this.parallel = [false, false, false];
            // Only a component exactly zero is parallel: any other one, however
            // small, uses its reciprocal, so no hit is lost to the slab test.
            for (let a = 0; a < 3; a++) {
                this.parallel[a] = direction[a] === 0.0;
                this.inverse[a] = this.parallel[a] ? 0.0 : 1.0 / direction[a]!;
            }
        }

        /** Entry parameter of the ray in the node box clipped to [0, tMax],
        or -1 on a miss or an empty box (`casSlab`) */
        public entry(node: BinnedSahHierarchyNode, tMax: number): number {
            let tNear = 0.0;
            let tFar = tMax;
            for (let a = 0; a < 3; a++) {
                // Empty box (the negative box of an empty instance or mesh): the
                // swap below would otherwise turn it into a box that is entered.
                if (node.minimum[a]! > node.maximum[a]!) {
                    return -1.0;
                }
                if (this.parallel[a]) {
                    if (this.origin[a]! < node.minimum[a]! || this.origin[a]! > node.maximum[a]!) {
                        return -1.0;
                    }
                    continue;
                }
                let t0 = (node.minimum[a]! - this.origin[a]!) * this.inverse[a]!;
                let t1 = (node.maximum[a]! - this.origin[a]!) * this.inverse[a]!;
                if (t0 > t1) {
                    const swap = t0;
                    t0 = t1;
                    t1 = swap;
                }
                tNear = t0 > tNear ? t0 : tNear;
                tFar = t1 < tFar ? t1 : tFar;
                if (tNear > tFar) {
                    return -1.0;
                }
            }
            return tNear;
        }
    }
}

//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =

package vsdk.toolkit.environment.geometry.boundingVolumeHierarchy;

import java.util.ArrayList;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.statistics.RaytraceStatistics;
import vsdk.toolkit.environment.geometry.element.Ray;

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

Java adaptation: nodes and the primitive order are kept in arrays (no
boxing during traversal), and the traversal stack is a `TraversalStack`
that a caller may keep and reuse, so a query allocates nothing.
*/
public final class BinnedSahHierarchy {
    public static final int BIN_COUNT = 12;
    public static final int MAXIMUM_DEPTH = 30;
    /** Leaf size of `build` when none is given (bottom level) */
    public static final int DEFAULT_MAXIMUM_LEAF_SIZE = 4;
    /** Traversal stack; it never holds more than `MAXIMUM_DEPTH + 1`
    entries */
    public static final int STACK_SIZE = 64;

    private BinnedSahHierarchyNode[] nodes;
    private int nodeCount;
    private int[] primitiveOrder;
    private int primitiveCount;

    /**
    Axis aligned box of the build. The default box is the "negative box"
    [+inf, -inf] of [WALD2007].3.4: enclosing it in another box leaves that
    box unchanged, so bins and partitions grow without emptiness checks.
    */
    private static final class BinnedSahBox {
        final double[] minimum = new double[3];
        final double[] maximum = new double[3];

        BinnedSahBox() {
            for ( int a = 0; a < 3; a++ ) {
                minimum[a] = 1e30;
                maximum[a] = -1e30;
            }
        }

        boolean isEmpty() {
            return minimum[0] > maximum[0] || minimum[1] > maximum[1] || minimum[2] > maximum[2];
        }

        void enclose(BinnedSahBox other) {
            for ( int a = 0; a < 3; a++ ) {
                minimum[a] = other.minimum[a] < minimum[a] ? other.minimum[a] : minimum[a];
                maximum[a] = other.maximum[a] > maximum[a] ? other.maximum[a] : maximum[a];
            }
        }

        void enclosePoint(double[] point) {
            for ( int a = 0; a < 3; a++ ) {
                minimum[a] = point[a] < minimum[a] ? point[a] : minimum[a];
                maximum[a] = point[a] > maximum[a] ? point[a] : maximum[a];
            }
        }

        double centroid(int axis) {
            return 0.5 * (minimum[axis] + maximum[axis]);
        }

        double extent(int axis) {
            return maximum[axis] - minimum[axis];
        }

        /** Half the surface area: the SAH cost of [WALD2007].3.1 drops the
        constant factors and only compares ratios */
        double halfArea() {
            if ( isEmpty() ) {
                return 0.0;
            }
            final double dx = maximum[0] - minimum[0];
            final double dy = maximum[1] - minimum[1];
            final double dz = maximum[2] - minimum[2];
            return dx * dy + dy * dz + dz * dx;
        }
    }

    /** Pending node of the top-down build */
    private static final class BinnedSahBuildTask {
        int node;
        int first;
        int count;
        int depth;
    }

    /** Best partition found by findSplit */
    private static final class BinnedSahSplit {
        int axis;
        int bin;
        double cost;
    }

    /** Slab entry parameters of a ray, shared by the traversals */
    public static final class RaySlab {
        public final double[] origin = new double[3];
        public final double[] inverse = new double[3];
        public final boolean[] parallel = new boolean[3];

        public RaySlab() {
        }

        public RaySlab(Ray ray) {
            set(ray);
        }

        public void set(Ray ray) {
            final Vector3Dd o = ray.getOrigin();
            final Vector3Dd d = ray.getDirection();
            origin[0] = o.x();
            origin[1] = o.y();
            origin[2] = o.z();
            final double dx = d.x();
            final double dy = d.y();
            final double dz = d.z();
            // Only a component exactly zero is parallel: any other one, however
            // small, uses its reciprocal, so no hit is lost to the slab test.
            parallel[0] = dx == 0.0;
            inverse[0] = parallel[0] ? 0.0 : 1.0 / dx;
            parallel[1] = dy == 0.0;
            inverse[1] = parallel[1] ? 0.0 : 1.0 / dy;
            parallel[2] = dz == 0.0;
            inverse[2] = parallel[2] ? 0.0 : 1.0 / dz;
        }

        /** Entry parameter of the ray in the node box clipped to [0, tMax],
        or -1 on a miss or an empty box (`casSlab`) */
        public double entry(BinnedSahHierarchyNode node, double tMax) {
            double tNear = 0.0;
            double tFar = tMax;
            for ( int a = 0; a < 3; a++ ) {
                // Empty box (the negative box of an empty instance or mesh): the
                // swap below would otherwise turn it into a box that is entered.
                if ( node.minimum[a] > node.maximum[a] ) {
                    return -1.0;
                }
                if ( parallel[a] ) {
                    if ( origin[a] < node.minimum[a] || origin[a] > node.maximum[a] ) {
                        return -1.0;
                    }
                    continue;
                }
                double t0 = (node.minimum[a] - origin[a]) * inverse[a];
                double t1 = (node.maximum[a] - origin[a]) * inverse[a];
                if ( t0 > t1 ) {
                    final double swap = t0;
                    t0 = t1;
                    t1 = swap;
                }
                tNear = t0 > tNear ? t0 : tNear;
                tFar = t1 < tFar ? t1 : tFar;
                if ( tNear > tFar ) {
                    return -1.0;
                }
            }
            return tNear;
        }
    }

    /**
    Java adaptation: the stacks and the slab of one traversal, which the C++
    port keeps on the call stack. Caller-owned, one per worker.
    */
    public static final class TraversalStack {
        final int[] nodeStack = new int[STACK_SIZE];
        final double[] distanceStack = new double[STACK_SIZE];
        final RaySlab slab = new RaySlab();
    }

    /** Adapts a closest hit visitor, which never stops, to the shared loop */
    private static final class NeverStoppingVisitor implements BinnedSahStoppingLeafVisitor {
        private final BinnedSahLeafVisitor visitor;

        NeverStoppingVisitor(BinnedSahLeafVisitor inVisitor) {
            visitor = inVisitor;
        }

        @Override
        public boolean visit(int primitive, double[] inOutBestT) {
            visitor.visit(primitive, inOutBestT);
            return false;
        }
    }

    public BinnedSahHierarchy() {
        nodes = new BinnedSahHierarchyNode[0];
        nodeCount = 0;
        primitiveOrder = new int[0];
        primitiveCount = 0;
    }

    private static void setNodeBounds(BinnedSahHierarchyNode node, BinnedSahBox box) {
        for ( int a = 0; a < 3; a++ ) {
            node.minimum[a] = box.minimum[a];
            node.maximum[a] = box.maximum[a];
        }
    }

    private static int binOf(double centroid, double lo, double scale) {
        // [WALD2007].3.3 Triangle-to-bin projection binID = k1 (c - k0); the
        // clamp replaces the (1 - epsilon) factor for centroids on the upper
        // bound of the centroid box.
        int bin = (int)((centroid - lo) * scale);
        if ( bin < 0 ) {
            bin = 0;
        }
        if ( bin > BIN_COUNT - 1 ) {
            bin = BIN_COUNT - 1;
        }
        return bin;
    }

    private static boolean findSplit(
        BinnedSahBox[] boxes,
        int[] order,
        int first,
        int count,
        BinnedSahBox centroidBox,
        BinnedSahSplit split) {
        final int bins = BIN_COUNT;
        boolean found = false;
        for ( int axis = 0; axis < 3; axis++ ) {
            final double extent = centroidBox.extent(axis);
            if ( !(extent > 0.0) ) {
                continue;
            }
            // [WALD2007].3.1 Bins of equal width over the centroid bounds; all
            // three axes are tried, [WALD2007] bins only the widest one.
            final double lo = centroidBox.minimum[axis];
            final double scale = (double)bins / extent;

            // [WALD2007].3.4 Bin updates: count and full 3D bounds per bin,
            // starting from a negative box.
            BinnedSahBox[] binBoxes = new BinnedSahBox[BIN_COUNT];
            int[] binCounts = new int[BIN_COUNT];
            for ( int b = 0; b < bins; b++ ) {
                binBoxes[b] = new BinnedSahBox();
                binCounts[b] = 0;
            }
            for ( int i = first; i < first + count; i++ ) {
                final BinnedSahBox box = boxes[order[i]];
                final int b = binOf(box.centroid(axis), lo, scale);
                binCounts[b]++;
                binBoxes[b].enclose(box);
            }

            // [WALD2007].3.5 Plane evaluations: sweep from the right, then from
            // the left; the cost of a split after bin `b` is
            // countLeft * areaLeft + countRight * areaRight ([WALD2007].3.1).
            // Partitions with an empty side are rejected.
            double[] rightArea = new double[BIN_COUNT];
            int[] rightCount = new int[BIN_COUNT];
            BinnedSahBox accumulated = new BinnedSahBox();
            int accumulatedCount = 0;
            for ( int b = bins - 1; b > 0; b-- ) {
                accumulated.enclose(binBoxes[b]);
                accumulatedCount += binCounts[b];
                rightArea[b] = accumulated.halfArea();
                rightCount[b] = accumulatedCount;
            }
            accumulated = new BinnedSahBox();
            accumulatedCount = 0;
            for ( int b = 0; b < bins - 1; b++ ) {
                accumulated.enclose(binBoxes[b]);
                accumulatedCount += binCounts[b];
                if ( accumulatedCount == 0 || rightCount[b + 1] == 0 ) {
                    continue;
                }
                final double cost = (double)accumulatedCount * accumulated.halfArea() +
                    (double)rightCount[b + 1] * rightArea[b + 1];
                if ( !found || cost < split.cost ) {
                    found = true;
                    split.cost = cost;
                    split.axis = axis;
                    split.bin = b;
                }
            }
        }
        return found;
    }

    private BinnedSahHierarchyNode addNode() {
        BinnedSahHierarchyNode node = new BinnedSahHierarchyNode();
        setNodeBounds(node, new BinnedSahBox());
        node.leftOrFirst = 0;
        node.count = 0;
        nodes[nodeCount] = node;
        nodeCount++;
        return node;
    }

    public void build(ArrayList<Double> boxValues) {
        build(boxValues, DEFAULT_MAXIMUM_LEAF_SIZE);
    }

    /**
    @param boxValues six values per primitive: lo x, y, z, hi x, y, z. Callers
    pad the boxes so rounding never hides a hit. An empty box (lo > hi)
    is allowed: its primitive is never reached.
    @param maximumLeafSize leaves are split while they hold more primitives
    than this and a split is cheaper (SAH), or while they hold more than
    twice this; values below 1 count as 1. Top levels over instances use 1,
    as a hardware TLAS.
    */
    public void build(ArrayList<Double> boxValues, int maximumLeafSize) {
        double[] values = new double[boxValues.size()];
        for ( int i = 0; i < values.length; i++ ) {
            values[i] = boxValues.get(i);
        }
        build(values, maximumLeafSize);
    }

    /**
    As `build(ArrayList, int)`, from the box values in an array
    */
    public void build(double[] boxValues, int maximumLeafSize) {
        if ( maximumLeafSize < 1 ) {
            maximumLeafSize = 1;
        }
        primitiveCount = boxValues.length / 6;

        // [WALD2007].3.2 Initial setup: the box of every primitive, given by
        // the caller; the build only looks at these boxes.
        BinnedSahBox[] boxes = new BinnedSahBox[primitiveCount];
        for ( int i = 0; i < primitiveCount; i++ ) {
            BinnedSahBox box = new BinnedSahBox();
            for ( int a = 0; a < 3; a++ ) {
                box.minimum[a] = boxValues[6 * i + a];
                box.maximum[a] = boxValues[6 * i + 3 + a];
            }
            boxes[i] = box;
        }

        // [WALD2007].3.6 Nodes and primitive ids in two arrays, at most
        // 2N - 1 nodes, ids initialized to 0, 1, 2, ...
        primitiveOrder = new int[primitiveCount];
        for ( int i = 0; i < primitiveCount; i++ ) {
            primitiveOrder[i] = i;
        }

        nodes = new BinnedSahHierarchyNode[primitiveCount > 0 ? 2 * primitiveCount : 1];
        nodeCount = 0;
        addNode();
        if ( primitiveCount == 0 ) {
            // One empty leaf: its negative box never passes the slab test.
            return;
        }

        ArrayList<BinnedSahBuildTask> stack = new ArrayList<>();
        BinnedSahBuildTask task = new BinnedSahBuildTask();
        task.node = 0;
        task.first = 0;
        task.count = primitiveCount;
        task.depth = 1;
        stack.add(task);

        int[] scratch = new int[primitiveCount];
        BinnedSahSplit split = new BinnedSahSplit();
        while ( stack.size() > 0 ) {
            task = stack.remove(stack.size() - 1);

            // [WALD2007].3.2 Node bounds and centroid bounds of the range
            // (recomputed per node instead of tracked while partitioning).
            BinnedSahBox bounds = new BinnedSahBox();
            BinnedSahBox centroidBox = new BinnedSahBox();
            double[] c = new double[3];
            for ( int i = task.first; i < task.first + task.count; i++ ) {
                final BinnedSahBox box = boxes[primitiveOrder[i]];
                bounds.enclose(box);
                for ( int a = 0; a < 3; a++ ) {
                    c[a] = box.centroid(a);
                }
                centroidBox.enclosePoint(c);
            }
            setNodeBounds(nodes[task.node], bounds);
            nodes[task.node].leftOrFirst = task.first;
            nodes[task.node].count = task.count;

            // [WALD2007].3.8 Termination a): few primitives. The depth cap is
            // not in [WALD2007]: it bounds the traversal stack.
            if ( task.count <= maximumLeafSize || task.depth >= MAXIMUM_DEPTH ) {
                continue;
            }

            split.axis = 0;
            split.bin = 0;
            split.cost = 0.0;
            int leftCount = 0;
            if ( findSplit(boxes, primitiveOrder, task.first, task.count, centroidBox, split) ) {
                final int axis = split.axis;
                final int splitBin = split.bin;
                final double leafCost = (double)task.count * bounds.halfArea();
                // [WALD2007].3.8 Termination c): the split costs more than the
                // leaf (only for nodes small enough to stay a leaf).
                if ( split.cost >= leafCost && task.count <= 2 * maximumLeafSize ) {
                    continue;
                }
                // [WALD2007].3.6 ID list partitioning with a temporary array;
                // stable: members of bins [0, splitBin] first.
                final double lo = centroidBox.minimum[axis];
                final double scale = (double)BIN_COUNT / centroidBox.extent(axis);
                int scratchSize = 0;
                for ( int i = task.first; i < task.first + task.count; i++ ) {
                    final int p = primitiveOrder[i];
                    if ( binOf(boxes[p].centroid(axis), lo, scale) <= splitBin ) {
                        primitiveOrder[task.first + leftCount] = p;
                        leftCount++;
                    }
                    else {
                        scratch[scratchSize] = p;
                        scratchSize++;
                    }
                }
                for ( int i = 0; i < scratchSize; i++ ) {
                    primitiveOrder[task.first + leftCount + i] = scratch[i];
                }
            }
            else {
                // Every centroid is the same point: [WALD2007].3.8 b) would make
                // a leaf; the range is split in halves so the leaf size stays
                // bounded.
                leftCount = task.count / 2;
            }

            final int left = nodeCount;
            addNode();
            addNode();
            nodes[task.node].leftOrFirst = left;
            nodes[task.node].count = 0;

            // Right first, so the left subtree is flattened first (depth first).
            BinnedSahBuildTask rightTask = new BinnedSahBuildTask();
            rightTask.node = left + 1;
            rightTask.first = task.first + leftCount;
            rightTask.count = task.count - leftCount;
            rightTask.depth = task.depth + 1;
            stack.add(rightTask);
            BinnedSahBuildTask leftTask = new BinnedSahBuildTask();
            leftTask.node = left;
            leftTask.first = task.first;
            leftTask.count = leftCount;
            leftTask.depth = task.depth + 1;
            stack.add(leftTask);
        }
    }

    /**
    The traversal loop shared by traverseClosest and traverse, on a non empty
    hierarchy: a stack, nearest child first, nodes farther than the closest
    hit skipped. Adds node visits and leaf entries to nodeVisits[0] and
    entryTests[0]; returns false on stack overflow.
    */
    private boolean traverseNodes(
        Ray ray,
        BinnedSahStoppingLeafVisitor visitor,
        double[] inOutBestT,
        long[] nodeVisitCounter,
        long[] entryTestCounter,
        TraversalStack stack) {
        final BinnedSahHierarchyNode[] nodeArray = nodes;
        final RaySlab slab = stack.slab;
        slab.set(ray);
        final double rootDistance = slab.entry(nodeArray[0], inOutBestT[0]);
        if ( rootDistance < 0.0 ) {
            return true;
        }
        final int[] nodeStack = stack.nodeStack;
        final double[] distanceStack = stack.distanceStack;
        long nodeVisits = 0;
        long entryTests = 0;
        int sp = 0;
        nodeStack[sp] = 0;
        distanceStack[sp] = rootDistance;
        sp++;
        boolean completed = true;
        while ( sp > 0 ) {
            sp--;
            if ( distanceStack[sp] > inOutBestT[0] ) {
                continue;
            }
            final BinnedSahHierarchyNode node = nodeArray[nodeStack[sp]];
            nodeVisits++;
            if ( node.count > 0 ) {
                boolean stopped = false;
                for ( int i = node.leftOrFirst; i < node.leftOrFirst + node.count; i++ ) {
                    if ( visitor.visit(primitiveOrder[i], inOutBestT) ) {
                        entryTests += i - node.leftOrFirst + 1;
                        stopped = true;
                        break;
                    }
                }
                if ( stopped ) {
                    break;
                }
                entryTests += node.count;
                continue;
            }
            int left = node.leftOrFirst;
            int right = left + 1;
            double dl = slab.entry(nodeArray[left], inOutBestT[0]);
            double dr = slab.entry(nodeArray[right], inOutBestT[0]);
            // Push the farther child first, so the nearer one is visited first.
            if ( dl >= 0.0 && dr >= 0.0 && dl < dr ) {
                final int swapNode = left;
                left = right;
                right = swapNode;
                final double swapDistance = dl;
                dl = dr;
                dr = swapDistance;
            }
            if ( sp + 2 > STACK_SIZE ) {
                completed = false;
                break;
            }
            if ( dl >= 0.0 ) {
                nodeStack[sp] = left;
                distanceStack[sp] = dl;
                sp++;
            }
            if ( dr >= 0.0 ) {
                nodeStack[sp] = right;
                distanceStack[sp] = dr;
                sp++;
            }
        }
        nodeVisitCounter[0] += nodeVisits;
        entryTestCounter[0] += entryTests;
        return completed;
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
    */
    public boolean traverseClosest(Ray ray, BinnedSahLeafVisitor visitor, double[] inOutBestT) {
        if ( nodeCount == 0 || primitiveCount == 0 ) {
            return true;
        }
        // Counted locally, reported once: no shared write per node.
        long[] nodeVisits = new long[] {0};
        long[] entryTests = new long[] {0};
        final boolean completed = traverseNodes(ray, new NeverStoppingVisitor(visitor),
            inOutBestT, nodeVisits, entryTests, new TraversalStack());
        RaytraceStatistics.recordInstanceLevelTraversal(nodeVisits[0], entryTests[0]);
        return completed;
    }

    /**
    The traversal of `traverseClosest` for callers that keep their own
    statistics and may stop early: the visitor ends the traversal by
    returning true, and node visits and leaf entries are added to the given
    counters instead of being reported to `RaytraceStatistics` (nothing
    static is written, so concurrent callers share the hierarchy freely).
    Node entry is clipped at parameter zero: a caller with a minimum
    parameter applies it in its visitor. Returns false when the stack would
    overflow, as `traverseClosest`.
    */
    public boolean traverse(
        Ray ray,
        BinnedSahStoppingLeafVisitor visitor,
        double[] inOutBestT,
        long[] nodeVisits,
        long[] entryTests) {
        return traverse(ray, visitor, inOutBestT, nodeVisits, entryTests, new TraversalStack());
    }

    /**
    As `traverse`, with a caller-owned traversal stack (Java adaptation)
    */
    public boolean traverse(
        Ray ray,
        BinnedSahStoppingLeafVisitor visitor,
        double[] inOutBestT,
        long[] nodeVisits,
        long[] entryTests,
        TraversalStack stack) {
        if ( nodeCount == 0 || primitiveCount == 0 ) {
            return true;
        }
        return traverseNodes(ray, visitor, inOutBestT, nodeVisits, entryTests, stack);
    }

    /** The first getNodeCount() entries are the nodes */
    public BinnedSahHierarchyNode[] getNodes() {
        return nodes;
    }

    /** Primitive ids in leaf order */
    public int[] getPrimitiveOrder() {
        return primitiveOrder;
    }

    public long getNodeCount() {
        return nodeCount;
    }

    /** Depth of the hierarchy (a single leaf has depth 1) */
    public int getDepth() {
        if ( nodeCount == 0 ) {
            return 0;
        }
        ArrayList<Integer> pendingNodes = new ArrayList<>();
        ArrayList<Integer> pendingDepths = new ArrayList<>();
        pendingNodes.add(0);
        pendingDepths.add(1);
        int maximum = 0;
        while ( pendingNodes.size() > 0 ) {
            final int last = pendingNodes.size() - 1;
            final int node = pendingNodes.remove(last);
            final int d = pendingDepths.remove(last);
            if ( d > maximum ) {
                maximum = d;
            }
            // Only the root of an empty hierarchy is a leaf without primitives.
            if ( nodes[node].count == 0 && nodeCount > 1 ) {
                pendingNodes.add(nodes[node].leftOrFirst);
                pendingDepths.add(d + 1);
                pendingNodes.add(nodes[node].leftOrFirst + 1);
                pendingDepths.add(d + 1);
            }
        }
        return maximum;
    }
}

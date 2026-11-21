//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/statistics/RaytraceStatistics.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/BinnedSahHierarchy.h"

// C++ port note: not inline (it is in the cpp port), as Turbo C++ 3.0
// rejects a loop in an inline constructor ("Expression syntax").
BinnedSahHierarchy::RaySlab::RaySlab(const Ray &ray)
{
    const Vector3Dd &o = ray.getOrigin();
    const Vector3Dd &d = ray.getDirection();
    origin[0] = o.x(); origin[1] = o.y(); origin[2] = o.z();
    // C++ port note: filled one by one, without an aggregate initializer of
    // non constant values
    double direction[3];
    direction[0] = d.x(); direction[1] = d.y(); direction[2] = d.z();
    // Only a component exactly zero is parallel: any other one, however
    // small, uses its reciprocal, so no hit is lost to the slab test.
    int a;
    for (a = 0; a < 3; a++) {
        parallel[a] = direction[a] == 0.0;
        inverse[a] = parallel[a] ? 0.0 : 1.0 / direction[a];
    }
}

/**
Axis aligned box of the build. The default box is the "negative box"
[+inf, -inf] of [WALD2007].3.4: enclosing it in another box leaves that box
unchanged, so bins and partitions grow without emptiness checks.
*/
class _BinnedSahBox {
  public:
    double minimum[3];
    double maximum[3];

    _BinnedSahBox()
    {
        for (int a = 0; a < 3; a++) {
            minimum[a] = 1e30;
            maximum[a] = -1e30;
        }
    }

    BOOLEAN isEmpty() const
    {
        return minimum[0] > maximum[0] || minimum[1] > maximum[1] || minimum[2] > maximum[2];
    }

    void enclose(const _BinnedSahBox &other)
    {
        for (int a = 0; a < 3; a++) {
            minimum[a] = other.minimum[a] < minimum[a] ? other.minimum[a] : minimum[a];
            maximum[a] = other.maximum[a] > maximum[a] ? other.maximum[a] : maximum[a];
        }
    }

    void enclosePoint(const double *point)
    {
        for (int a = 0; a < 3; a++) {
            minimum[a] = point[a] < minimum[a] ? point[a] : minimum[a];
            maximum[a] = point[a] > maximum[a] ? point[a] : maximum[a];
        }
    }

    double centroid(int axis) const
    {
        return 0.5 * (minimum[axis] + maximum[axis]);
    }

    double extent(int axis) const
    {
        return maximum[axis] - minimum[axis];
    }

    /** Half the surface area: the SAH cost of [WALD2007].3.1 drops the
    constant factors and only compares ratios */
    double halfArea() const
    {
        if (isEmpty()) {
            return 0.0;
        }
        const double dx = maximum[0] - minimum[0];
        const double dy = maximum[1] - minimum[1];
        const double dz = maximum[2] - minimum[2];
        return dx * dy + dy * dz + dz * dx;
    }
};

/** Pending node of the top-down build */
class _BinnedSahBuildTask {
  public:
    int node;
    int first;
    int count;
    int depth;
};

static void
setNodeBounds(BinnedSahHierarchyNode &node, const _BinnedSahBox &box)
{
    for (int a = 0; a < 3; a++) {
        node.minimum[a] = box.minimum[a];
        node.maximum[a] = box.maximum[a];
    }
}

static int
binOf(double centroid, double lo, double scale)
{
    // [WALD2007].3.3 Triangle-to-bin projection binID = k1 (c - k0); the
    // clamp replaces the (1 - epsilon) factor for centroids on the upper
    // bound of the centroid box.
    int bin = (int)((centroid - lo) * scale);
    if (bin < 0) {
        bin = 0;
    }
    if (bin > BinnedSahHierarchy::BIN_COUNT - 1) {
        bin = BinnedSahHierarchy::BIN_COUNT - 1;
    }
    return bin;
}

static BOOLEAN
findSplit(const ArrayList<_BinnedSahBox> &boxes,
    const ArrayList<int> &order, int first, int count,
    const _BinnedSahBox &centroidBox, int &outAxis, int &outBin, double &outCost)
{
    const int BINS = BinnedSahHierarchy::BIN_COUNT;
    BOOLEAN found = FALSE;
    for (int axis = 0; axis < 3; axis++) {
        const double extent = centroidBox.extent(axis);
        if (!(extent > 0.0)) {
            continue;
        }
        // [WALD2007].3.1 Bins of equal width over the centroid bounds; all
        // three axes are tried, [WALD2007] bins only the widest one.
        const double lo = centroidBox.minimum[axis];
        const double scale = (double)BINS / extent;

        // [WALD2007].3.4 Bin updates: count and full 3D bounds per bin,
        // starting from a negative box.
        _BinnedSahBox binBoxes[BinnedSahHierarchy::BIN_COUNT];
        int binCounts[BinnedSahHierarchy::BIN_COUNT];
        int b;
        for (b = 0; b < BINS; b++) {
            binCounts[b] = 0;
        }
        for (int i = first; i < first + count; i++) {
            const _BinnedSahBox &box = boxes[order[i]];
            const int bin = binOf(box.centroid(axis), lo, scale);
            binCounts[bin]++;
            binBoxes[bin].enclose(box);
        }

        // [WALD2007].3.5 Plane evaluations: sweep from the right, then from
        // the left; the cost of a split after bin `b` is
        // countLeft * areaLeft + countRight * areaRight ([WALD2007].3.1).
        // Partitions with an empty side are rejected.
        double rightArea[BinnedSahHierarchy::BIN_COUNT];
        int rightCount[BinnedSahHierarchy::BIN_COUNT];
        _BinnedSahBox accumulated;
        int accumulatedCount = 0;
        for (b = BINS - 1; b > 0; b--) {
            accumulated.enclose(binBoxes[b]);
            accumulatedCount += binCounts[b];
            rightArea[b] = accumulated.halfArea();
            rightCount[b] = accumulatedCount;
        }
        accumulated = _BinnedSahBox();
        accumulatedCount = 0;
        for (b = 0; b < BINS - 1; b++) {
            accumulated.enclose(binBoxes[b]);
            accumulatedCount += binCounts[b];
            if (accumulatedCount == 0 || rightCount[b + 1] == 0) {
                continue;
            }
            const double cost = (double)accumulatedCount * accumulated.halfArea() +
                (double)rightCount[b + 1] * rightArea[b + 1];
            if (!found || cost < outCost) {
                found = TRUE;
                outCost = cost;
                outAxis = axis;
                outBin = b;
            }
        }
    }
    return found;
}

BinnedSahLeafVisitor::~BinnedSahLeafVisitor()
{
}

BinnedSahHierarchy::BinnedSahHierarchy()
{
}

void
BinnedSahHierarchy::build(const ArrayList<double> &boxValues,
    int maximumLeafSize)
{
    if (maximumLeafSize < 1) {
        maximumLeafSize = 1;
    }
    const int primitiveCount = (int)(boxValues.size() / 6);
    int i;

    // [WALD2007].3.2 Initial setup: the box of every primitive, given by
    // the caller; the build only looks at these boxes.
    ArrayList<_BinnedSahBox> boxes(primitiveCount > 0 ? primitiveCount : 1);
    for (i = 0; i < primitiveCount; i++) {
        _BinnedSahBox box;
        for (int a = 0; a < 3; a++) {
            box.minimum[a] = boxValues[6 * i + a];
            box.maximum[a] = boxValues[6 * i + 3 + a];
        }
        boxes.add(box);
    }

    // [WALD2007].3.6 Nodes and primitive ids in two arrays, at most
    // 2N - 1 nodes, ids initialized to 0, 1, 2, ...
    nodes.clear();
    primitiveOrder.clear();
    primitiveOrder.reserve(primitiveCount > 0 ? primitiveCount : 1);
    for (i = 0; i < primitiveCount; i++) {
        primitiveOrder.add(i);
    }

    BinnedSahHierarchyNode root;
    setNodeBounds(root, _BinnedSahBox());
    root.leftOrFirst = 0;
    root.count = 0;
    nodes.reserve(primitiveCount > 0 ? 2L * primitiveCount : 1);
    nodes.add(root);
    if (primitiveCount == 0) {
        // One empty leaf: its negative box never passes the slab test.
        return;
    }

    ArrayList<_BinnedSahBuildTask> stack;
    _BinnedSahBuildTask task;
    task.node = 0;
    task.first = 0;
    task.count = primitiveCount;
    task.depth = 1;
    stack.add(task);

    ArrayList<int> scratch(primitiveCount);
    while (stack.size() > 0) {
        task = stack[stack.size() - 1];
        stack.remove(stack.size() - 1);

        // [WALD2007].3.2 Node bounds and centroid bounds of the range
        // (recomputed per node instead of tracked while partitioning).
        _BinnedSahBox bounds;
        _BinnedSahBox centroidBox;
        for (i = task.first; i < task.first + task.count; i++) {
            const _BinnedSahBox &box = boxes[primitiveOrder[i]];
            bounds.enclose(box);
            double c[3];
            for (int a = 0; a < 3; a++) {
                c[a] = box.centroid(a);
            }
            centroidBox.enclosePoint(c);
        }
        setNodeBounds(nodes[task.node], bounds);
        nodes[task.node].leftOrFirst = task.first;
        nodes[task.node].count = task.count;

        // [WALD2007].3.8 Termination a): few primitives. The depth cap is
        // not in [WALD2007]: it bounds the traversal stack.
        if (task.count <= maximumLeafSize || task.depth >= MAXIMUM_DEPTH) {
            continue;
        }

        int axis = 0;
        int splitBin = 0;
        double splitCost = 0.0;
        int leftCount = 0;
        if (findSplit(boxes, primitiveOrder, task.first, task.count,
                centroidBox, axis, splitBin, splitCost)) {
            const double leafCost = (double)task.count * bounds.halfArea();
            // [WALD2007].3.8 Termination c): the split costs more than the
            // leaf (only for nodes small enough to stay a leaf).
            if (splitCost >= leafCost && task.count <= 2 * maximumLeafSize) {
                continue;
            }
            // [WALD2007].3.6 ID list partitioning with a temporary array;
            // stable: members of bins [0, splitBin] first.
            const double lo = centroidBox.minimum[axis];
            const double scale = (double)BIN_COUNT / centroidBox.extent(axis);
            scratch.clear();
            for (i = task.first; i < task.first + task.count; i++) {
                const int p = primitiveOrder[i];
                if (binOf(boxes[p].centroid(axis), lo, scale) <= splitBin) {
                    primitiveOrder[task.first + leftCount] = p;
                    leftCount++;
                }
                else {
                    scratch.add(p);
                }
            }
            for (long int k = 0; k < scratch.size(); k++) {
                primitiveOrder[task.first + leftCount + k] = scratch[k];
            }
        }
        else {
            // Every centroid is the same point: [WALD2007].3.8 b) would make
            // a leaf; the range is split in halves so the leaf size stays
            // bounded.
            leftCount = task.count / 2;
        }

        const int left = (int)nodes.size();
        BinnedSahHierarchyNode child;
        setNodeBounds(child, _BinnedSahBox());
        child.leftOrFirst = 0;
        child.count = 0;
        nodes.add(child);
        nodes.add(child);
        nodes[task.node].leftOrFirst = left;
        nodes[task.node].count = 0;

        // Right first, so the left subtree is flattened first (depth first).
        _BinnedSahBuildTask rightTask;
        rightTask.node = left + 1;
        rightTask.first = task.first + leftCount;
        rightTask.count = task.count - leftCount;
        rightTask.depth = task.depth + 1;
        stack.add(rightTask);
        _BinnedSahBuildTask leftTask;
        leftTask.node = left;
        leftTask.first = task.first;
        leftTask.count = leftCount;
        leftTask.depth = task.depth + 1;
        stack.add(leftTask);
    }

}

BOOLEAN
BinnedSahHierarchy::traverseClosest(const Ray &ray, BinnedSahLeafVisitor &visitor,
    double &inOutBestT) const
{
    if (nodes.size() == 0 || primitiveOrder.size() == 0) {
        return TRUE;
    }
    const RaySlab slab(ray);
    const BinnedSahHierarchyNode *nodeArray = nodes.data();
    const double rootDistance = slab.entry(nodeArray[0], inOutBestT);
    if (rootDistance < 0.0) {
        RaytraceStatistics::recordInstanceLevelTraversal(0, 0);
        return TRUE;
    }
    // Counted locally, reported once: no shared write per node.
    long int nodeVisits = 0;
    long int entryTests = 0;
    int nodeStack[STACK_SIZE];
    double distanceStack[STACK_SIZE];
    int sp = 0;
    nodeStack[sp] = 0;
    distanceStack[sp] = rootDistance;
    sp++;
    while (sp > 0) {
        sp--;
        if (distanceStack[sp] > inOutBestT) {
            continue;
        }
        const BinnedSahHierarchyNode &node = nodeArray[nodeStack[sp]];
        nodeVisits++;
        if (node.count > 0) {
            for (int i = node.leftOrFirst; i < node.leftOrFirst + node.count; i++) {
                visitor.visit(primitiveOrder[i], inOutBestT);
            }
            entryTests += node.count;
            continue;
        }
        int left = node.leftOrFirst;
        int right = left + 1;
        double dl = slab.entry(nodeArray[left], inOutBestT);
        double dr = slab.entry(nodeArray[right], inOutBestT);
        // Push the farther child first, so the nearer one is visited first.
        if (dl >= 0.0 && dr >= 0.0 && dl < dr) {
            const int swapNode = left;
            left = right;
            right = swapNode;
            const double swapDistance = dl;
            dl = dr;
            dr = swapDistance;
        }
        if (sp + 2 > STACK_SIZE) {
            RaytraceStatistics::recordInstanceLevelTraversal(nodeVisits, entryTests);
            return FALSE;
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
    RaytraceStatistics::recordInstanceLevelTraversal(nodeVisits, entryTests);
    return TRUE;
}

const ArrayList<BinnedSahHierarchyNode> &
BinnedSahHierarchy::getNodes() const
{
    return nodes;
}

const ArrayList<int> &
BinnedSahHierarchy::getPrimitiveOrder() const
{
    return primitiveOrder;
}

long int
BinnedSahHierarchy::getNodeCount() const
{
    return nodes.size();
}

int
BinnedSahHierarchy::getDepth() const
{
    if (nodes.size() == 0) {
        return 0;
    }
    ArrayList<int> pendingNodes;
    ArrayList<int> pendingDepths;
    pendingNodes.add(0);
    pendingDepths.add(1);
    int maximum = 0;
    while (pendingNodes.size() > 0) {
        const long int last = pendingNodes.size() - 1;
        const int node = pendingNodes[last];
        const int d = pendingDepths[last];
        pendingNodes.remove(last);
        pendingDepths.remove(last);
        if (d > maximum) {
            maximum = d;
        }
        // Only the root of an empty hierarchy is a leaf without primitives.
        if (nodes[node].count == 0 && nodes.size() > 1) {
            pendingNodes.add(nodes[node].leftOrFirst);
            pendingDepths.add(d + 1);
            pendingNodes.add(nodes[node].leftOrFirst + 1);
            pendingDepths.add(d + 1);
        }
    }
    return maximum;
}

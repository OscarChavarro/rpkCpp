package vsdk.toolkit.common.dataStructures;

import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.common.logging.Logger;

/** A three-dimensional tree. Radius and distances are squared distances. */
public final class KDTree<T> {
    public static final float KD_MAX_RADIUS = 1e10f;
    private final KDTreeAdapter<T> adapter;
    private final boolean copyData;
    private KDTreeNode<T> root;
    private BalancedKDTreeNode<T>[] balanced;
    private int numberOfNodes;
    private int unbalancedCount;
    private int balancedCount;
    private int firstLeaf;

    public KDTree(KDTreeAdapter<T> adapter, boolean copyData) {
        this.adapter = adapter;
        this.copyData = copyData;
    }

    public void addPoint(T value, short flags) {
        T stored = copyData ? adapter.copy(value) : value;
        KDTreeNode<T> node = new KDTreeNode<>(stored, flags);
        Vector3Df point = adapter.getKDPosition(stored);
        KDTreeNode<T> parent = null;
        KDTreeNode<T> current = root;
        while (current != null) {
            parent = current;
            int axis = current.discriminator();
            current = point.component(axis) <= adapter.getKDPosition(current.data).component(axis)
                ? current.loson : current.hison;
        }
        if (parent == null) {
            root = node;
        } else {
            Vector3Df parentPoint = adapter.getKDPosition(parent.data);
            if (parent.loson == null && parent.hison == null) {
                float dx = Math.abs(point.x() - parentPoint.x());
                float dy = Math.abs(point.y() - parentPoint.y());
                float dz = Math.abs(point.z() - parentPoint.z());
                int axis = dx > dy ? (dx > dz ? 0 : 2) : (dy > dz ? 1 : 2);
                parent.setDiscriminator(axis);
            }
            if (point.component(parent.discriminator()) <= parentPoint.component(parent.discriminator())) {
                parent.loson = node;
            } else {
                parent.hison = node;
            }
        }
        ++numberOfNodes;
        ++unbalancedCount;
    }

    public int query(Vector3Df point, int count, T[] results, float[] distances, float radius, short excluded) {
        if (count <= 0) return 0;
        if (distances == null && count > 1000) {
            Logger.reportMessage(this, Logger.ERROR, "KDTree::query", "Too many nodes requested");
            return 0;
        }
        KDQuery<T> q = new KDQuery<>();
        q.point = point;
        q.wanted = count;
        q.results = results;
        q.distances = distances == null ? new float[1000] : distances;
        q.maximumDistance = radius;
        q.squaredRadius = radius;
        q.excludeFlags = excluded;
        if (balanced != null) balancedQueryRec(0, q);
        if (root != null) queryRec(root, q);
        return q.found;
    }

    private float distance(T value, Vector3Df point) {
        Vector3Df position = adapter.getKDPosition(value);
        float d = position.x() - point.x();
        float result = d * d;
        d = position.y() - point.y();
        result += d * d;
        d = position.z() - point.z();
        result += d * d;
        return result;
    }

    private void consider(T data, int flags, KDQuery<T> q) {
        float dist = distance(data, q.point);
        if (dist >= q.maximumDistance || (flags & q.excludeFlags) != 0) return;
        if (q.found < q.wanted) {
            int child = q.found;
            q.results[child] = data;
            q.distances[child] = dist;
            while (child > 0) {
                int parent = (child - 1) >> 1;
                if (q.distances[parent] >= q.distances[child]) break;
                swapResults(q, parent, child);
                child = parent;
            }
            if (++q.found == q.wanted) q.maximumDistance = q.distances[0];
        } else {
            q.results[0] = data;
            q.distances[0] = dist;
            int parent = 0;
            int child = 1;
            while (child < q.found) {
                if (q.distances[child] <= q.distances[parent]) {
                    if (++child >= q.found || q.distances[child] <= q.distances[parent]) break;
                } else if (child + 1 < q.found && q.distances[child + 1] > q.distances[child]) {
                    ++child;
                }
                swapResults(q, parent, child);
                parent = child;
                child = (parent << 1) + 1;
            }
            q.maximumDistance = q.distances[0];
        }
    }

    private void swapResults(KDQuery<T> q, int a, int b) {
        T value = q.results[a]; q.results[a] = q.results[b]; q.results[b] = value;
        float dist = q.distances[a]; q.distances[a] = q.distances[b]; q.distances[b] = dist;
    }

    private void queryRec(KDTreeNode<T> node, KDQuery<T> q) {
        consider(node.data, node.flags, q);
        float d = adapter.getKDPosition(node.data).component(node.discriminator()) - q.point.component(node.discriminator());
        KDTreeNode<T> near = d >= 0 ? node.loson : node.hison;
        KDTreeNode<T> far = d >= 0 ? node.hison : node.loson;
        if (near != null) queryRec(near, q);
        d *= d;
        if (far != null && ((q.found < q.wanted && d < q.squaredRadius) || d < q.maximumDistance)) queryRec(far, q);
    }

    private void balancedQueryRec(int index, KDQuery<T> q) {
        BalancedKDTreeNode<T> node = balanced[index];
        if (index < firstLeaf) {
            int axis = node.discriminator();
            float d = adapter.getKDPosition(node.data).component(axis) - q.point.component(axis);
            int near = (index << 1) + 1;
            int far = near + 1;
            if (d < 0) { int temp = near; near = far; far = temp; }
            if (near < balancedCount) balancedQueryRec(near, q);
            d *= d;
            if (far < balancedCount && ((q.found < q.wanted && d < q.squaredRadius) || d < q.maximumDistance)) balancedQueryRec(far, q);
        }
        consider(node.data, node.flags(), q);
    }

    @SuppressWarnings("unchecked")
    public void balance() {
        if (unbalancedCount == 0) return;
        BalancedKDTreeNode<T>[] source = (BalancedKDTreeNode<T>[]) new BalancedKDTreeNode<?>[numberOfNodes];
        BalancedKDTreeNode<T>[] dest = (BalancedKDTreeNode<T>[]) new BalancedKDTreeNode<?>[numberOfNodes];
        int count = 0;
        for (int i = 0; i < balancedCount; ++i) source[count++] = balanced[i];
        int[] next = {count};
        copyNodes(root, source, next);
        balanceRec(source, dest, 0, 0, numberOfNodes - 1);
        balanced = dest;
        balancedCount = numberOfNodes;
        firstLeaf = (balancedCount + 1) / 2;
        root = null;
        unbalancedCount = 0;
    }

    private void copyNodes(KDTreeNode<T> node, BalancedKDTreeNode<T>[] dest, int[] index) {
        if (node == null) return;
        dest[index[0]++] = new BalancedKDTreeNode<>(node.data, node.flags & 0xfff0);
        copyNodes(node.loson, dest, index);
        copyNodes(node.hison, dest, index);
    }

    private void balanceRec(BalancedKDTreeNode<T>[] source, BalancedKDTreeNode<T>[] dest, int index, int low, int high) {
        if (low == high) { dest[index] = source[low]; dest[index].setDiscriminator(0); return; }
        int axis = bestAxis(source, low, high);
        int median = select(source, low, high, axis);
        dest[index] = source[median];
        dest[index].setDiscriminator(axis);
        if (low < median) balanceRec(source, dest, (index << 1) + 1, low, median - 1);
        if (high > median) balanceRec(source, dest, (index << 1) + 2, median + 1, high);
    }

    private int bestAxis(BalancedKDTreeNode<T>[] nodes, int low, int high) {
        float[] min = {Float.MAX_VALUE, Float.MAX_VALUE, Float.MAX_VALUE};
        float[] max = {-Float.MAX_VALUE, -Float.MAX_VALUE, -Float.MAX_VALUE};
        for (int i = low; i <= high; ++i) {
            Vector3Df p = adapter.getKDPosition(nodes[i].data);
            for (int axis = 0; axis < 3; ++axis) {
                float v = p.component(axis);
                if (v < min[axis]) min[axis] = v;
                if (v > max[axis]) max[axis] = v;
            }
        }
        int axis = 0;
        float spread = max[0] - min[0];
        if (max[1] - min[1] > spread) { axis = 1; spread = max[1] - min[1]; }
        if (max[2] - min[2] > spread) axis = 2;
        return axis;
    }

    private float value(BalancedKDTreeNode<T>[] nodes, int index, int axis) {
        return adapter.getKDPosition(nodes[index].data).component(axis);
    }
    private void swap(BalancedKDTreeNode<T>[] nodes, int a, int b) {
        BalancedKDTreeNode<T> temp = nodes[a]; nodes[a] = nodes[b]; nodes[b] = temp;
    }
    private int select(BalancedKDTreeNode<T>[] nodes, int low, int high, int axis) {
        int n = high - low + 1;
        int fl = (int)(Math.log(n + 0.1) / Math.log(2));
        int power = 1 << fl;
        int last = n - (power - 1);
        int half = power / 2;
        int median = low + (last < half ? last + half - 1 : half + half - 1);
        while (true) {
            if (high <= low) return median;
            if (high == low + 1) {
                if (value(nodes, low, axis) > value(nodes, high, axis)) swap(nodes, low, high);
                return median;
            }
            int middle = (low + high + 1) / 2;
            if (value(nodes, middle, axis) > value(nodes, high, axis)) swap(nodes, middle, high);
            if (value(nodes, low, axis) > value(nodes, high, axis)) swap(nodes, low, high);
            if (value(nodes, middle, axis) > value(nodes, low, axis)) swap(nodes, middle, low);
            swap(nodes, middle, low + 1);
            int ll = low + 1, hh = high;
            while (true) {
                do { ++ll; } while (value(nodes, low, axis) > value(nodes, ll, axis));
                do { --hh; } while (value(nodes, hh, axis) > value(nodes, low, axis));
                if (hh < ll) break;
                swap(nodes, ll, hh);
            }
            swap(nodes, low, hh);
            if (hh <= median) low = ll;
            if (hh >= median) high = hh - 1;
        }
    }

    public int getBalancedCount() { return balancedCount; }
    public int getUnbalancedCount() { return unbalancedCount; }
    public int getFirstLeaf() { return firstLeaf; }
    public BalancedKDTreeNode<T> getBalancedNode(int index) { return balanced[index]; }
    public void iterateNodes(KDTreeVisitor<T> visitor) {
        if (unbalancedCount > 0) {
            Logger.reportMessage(this, Logger.ERROR, "KDTree::iterateNodes", "Cannot iterate unbalanced trees");
            return;
        }
        for (int i = 0; i < balancedCount; ++i) visitor.visit(balanced[i].data);
    }
}

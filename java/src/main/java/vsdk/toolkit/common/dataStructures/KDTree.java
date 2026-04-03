package vsdk.toolkit.common.dataStructures;

/**
 KD Tree

Creation :

KDTree( int dimension, int dataSize, bool CopyData = true )

  dimension : the dimension k of the kd tree
  dataSize : size of the data blocks that are stored in the tree.
    The first k entries of the data block must be floats. These
    are the coordinates of this point in the kd space
  CopyData : boolean indicating if data must be copied.

virtual void addPoint(void *data)

  Adds a point to the tree. (First k entries of data == point)

Destruction :

virtual KDTree void )

  Destroys kd tree and nodes. Data is freed only when
  copy data was true.
 
Interrogation :

virtual int query(const float *point, int N, void *results,
	     float *distances = nullptr, float radius = HUGE_DOUBLE_VALUE)

 Gives a maximum of N positions that are closest to the query point
 ('point'). An optional radius defines the maximum distance
to the query positions. The number of positions found is returned.

float *point : the query point (float point[k])
int N : maximum number of positions to return
void *results : array of N pointers where the results of the query
                will be stored. (DATATYPE *results[k])
float *distances : array of N floats where the distances to the
                   query point can be stored. If nullptr no distances
                   are stored.
float radius : defines the maximum allowed distance between any point 
               found and the query point. 

Ref : - Bentley, J.L. (1975) Multidimensional search trees used for 
        associative searching. Comm. of the ACM 18(9) p. 509-517
      - Friedman et al. (1977) An algorithm for finding best matches
        in logarithmic expected time. ACM Trans. on Math. Software 3(3)
        p. 209-226
*/

// Start of non balanced part of the kd tree

// (numBalanced+1) / 2 : index of first leaf element

// Start of balanced part of the kd tree

// Unbalanced part

// Balanced part

// Not HUGE_DOUBLE_VALUE, since we need to square it.

// KD Tree with one data element per node

// Maximum 1000!

/**
Add a point in the kd tree, this is always to the unbalanced part
*/

// Add the point to the unbalanced part

// Default

// The parent

// Test discriminator

// Choose an appropriate discriminator for the parent

// Choose correct side

// Parent is nullptr or discriminator is fixed...

/**
Iterate nodes : iterate all nodes (only for balanced trees!)
*/

/**
Query the kd tree : both balanced and unbalanced parts taken into
account ! (The balanced part is searched first)
*/

// Fill in static class data

// First query balanced part

// Now query unbalanced part using the already found nodes

// from the balanced part

/**
Distance calculation
*/

/**
Max heap stuff
Adapted from patched POVRAY (megasrc), who took it from Sejwick
*/

// Ripple the node (qdat_s.foundN) upward. There are qdat_s.foundN + 1 nodes

// in the tree

// Root of tree == index 0 so parent = any son - 1 / 2

// If all the photons are filled, we can use the actual maximum distance

// Ripple the top node, which may not be max anymore downwards

// There are qdat_s.foundN nodes in the tree, starting at index 0

// Node in place, left son and right son smaller

// Take maximum of the two sons

// Swap because son > parent

// Top = maximum element. Replace it with new and ripple down

// The heap is full (foundN == wantedN), but this is not required

// Top

// Max = top of heap

/**
Query_rec for the unbalanced kd tree part
*/

// Add this point anyway, because we haven't got enough positions yet.

// We have to check for the radius only here, since if N positions

// are added, maximumDistance <= radius

// Add point if distance < maximumDistance

// Reuse distance

// Always call near node recursively

// Square distance to the separator plane

// Discriminator line closer than maximumDistance : nearer positions can lie

// on the far side. Or there are still not enough nodes found

// Recursive call to the child nodes

// Test discr (reuse distance)

// node loson

// node hison

// qdat_s.foundN < qdat_s.wantedN

/**
This Quick select routine is adapted from the algorithm described in
"Numerical recipes in C", Second Edition,
Cambridge University Press, 1992, Section 8.5, ISBN 0-521-43108-5
*/

// High inclusive

// Add 0.1 because integer powers of 2 sometimes gave a smaller FL (8 -> 2)

// 2^FL

// Number of elements on last level

// Half the room for elements on last level

// All in left subtree

// Full bottom level in left subtree

// Rest in right subtree

/**
Return index of median element
*/

// One element only

// Two elements only

// Find median of low, middle and high volumeListsOfItems; swap into position low

// Swap low item (now in position middle) into position (low + 1)

// Nibble from each end towards middle, swapping volumeListsOfItems when stuck

// Swap middle item (in position low) back into correct position

// Re-set active partition

/**
Balance the kd tree
*/

// X spread

/**
balance the tree recursively
*/

// High inclusive!

// Put it in dest

// Don't care

// Find the balance median element

// Recurse low and high array

/**
Balance the tree, it is possible that a part is already balanced!
*/

// Make an unsorted BalancedKDTreeNode array pointing to the nodes

// No balancing needed.

// Copy balanced

// Copy unbalanced

// Clear old balanced and unbalanced part (but no data delete)

// Could we do with just 1 array???

// Now balance the tree recursively

import java.util.Arrays;
import vsdk.toolkit.common.linealAlgebra.Numeric;

public class KDTree {
    public interface NodeCallback {
        void call(Object userData, Object nodeData);
    }

    protected int numberOfNodes;
    protected int dataSize;
    protected int numUnbalanced;
    protected KDTreeNode root;
    protected int numBalanced;
    protected int firstLeaf;
    protected BalancedKDTreeNode[] balancedRootNode;
    protected boolean copyData;
    protected static float[] distances;

    public static final float KD_MAX_RADIUS = 1e10f;

    public KDTree(int dataSize) {
        this(dataSize, true);
    }

    public KDTree(int inDataSize, boolean copyData) {
        this.dataSize = inDataSize;
        numberOfNodes = 0;
        numUnbalanced = 0;
        root = null;
        numBalanced = 0;
        balancedRootNode = null;
        this.copyData = copyData;

        if (distances == null) {
            distances = new float[1000];
        }
        firstLeaf = 0;
    }

    public void dispose() {
        deleteNodes(root, copyData);
        root = null;
        deleteBNodes(copyData);
    }

    public void addPoint(Object data) {
        addPoint(data, (short)0);
    }

    public void addPoint(Object data, short flags) {
        KDTreeNode newNode = new KDTreeNode();
        newNode.mData = assignData(data);
        newNode.mFlags = flags;
        newNode.setDiscriminator(0);
        newNode.loson = null;
        newNode.hison = null;

        float[] newPoint = asPoint(data);

        numberOfNodes++;
        numUnbalanced++;

        KDTreeNode parent = null;
        KDTreeNode current = root;

        while (current != null) {
            parent = current;
            int discriminator = parent.discriminator();
            float[] parentPoint = asPoint(parent.mData);
            if (newPoint[discriminator] <= parentPoint[discriminator]) {
                current = parent.loson;
            }
            else {
                current = parent.hison;
            }
        }

        if ((parent != null) && (parent.loson == null) && (parent.hison == null)) {
            float[] parentPoint = asPoint(parent.mData);
            float dx = Math.abs(newPoint[0] - parentPoint[0]);
            float dy = Math.abs(newPoint[1] - parentPoint[1]);
            float dz = Math.abs(newPoint[2] - parentPoint[2]);

            int discriminator;
            if (dx > dy) {
                discriminator = (dx > dz) ? 0 : 2;
            }
            else {
                discriminator = (dy > dz) ? 1 : 2;
            }

            parent.setDiscriminator(discriminator);
            if (newPoint[discriminator] <= parentPoint[discriminator]) {
                parent.loson = newNode;
            }
            else {
                parent.hison = newNode;
            }
        }
        else if (parent == null) {
            root = newNode;
        }
        else {
            int discriminator = parent.discriminator();
            float[] parentPoint = asPoint(parent.mData);
            if (newPoint[discriminator] <= parentPoint[discriminator]) {
                parent.loson = newNode;
            }
            else {
                parent.hison = newNode;
            }
        }
    }

    public void iterateNodes(NodeCallback callback, Object data) {
        if (numUnbalanced > 0) {
            vsdk.toolkit.common.Error.error("KDTree::iterateNodes", "Cannot iterate unbalanced trees");
            return;
        }

        for (int i = 0; i < numBalanced; i++) {
            callback.call(data, balancedRootNode[i].mData);
        }
    }

    public int query(float[] point, int n, Object[] results) {
        return query(point, n, results, null, KD_MAX_RADIUS, (short)0);
    }

    public int query(float[] point, int n, Object[] results, float[] inDistances, float radius, short excludeFlags) {
        KDQuery queryData = new KDQuery();

        float[] usedDistances;
        if (inDistances == null) {
            if (n > distances.length) {
                vsdk.toolkit.common.Error.error("KDTree::query", "Too many nodes requested");
                return 0;
            }
            usedDistances = distances;
        }
        else {
            usedDistances = inDistances;
        }

        queryData.point = point;
        queryData.wantedN = n;
        queryData.foundN = 0;
        queryData.results = results;
        queryData.distances = usedDistances;
        queryData.maximumDistance = radius;
        queryData.sqrRadius = radius;
        queryData.excludeFlags = excludeFlags;
        queryData.notFilled = true;

        if (balancedRootNode != null) {
            balancedQueryRec(0, queryData);
        }

        if (root != null) {
            queryRec(root, queryData);
        }

        return queryData.foundN;
    }

    public void balance() {
        if (numUnbalanced == 0) {
            return;
        }

        System.err.printf("Balancing kd-tree: %d nodes...%n", numberOfNodes);

        BalancedKDTreeNode[] broot = new BalancedKDTreeNode[numberOfNodes + 1];
        for (int i = 0; i < broot.length; i++) {
            broot[i] = new BalancedKDTreeNode();
        }
        broot[numberOfNodes].mData = new float[] {Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY};
        broot[numberOfNodes].mFlags = 128;

        int index = 0;
        for (int i = 0; i < numBalanced; i++) {
            broot[index++] = balancedRootNode[i];
        }

        int[] pIndex = new int[] {index};
        copyUnbalancedRec(root, broot, pIndex);

        deleteNodes(root, false);
        root = null;
        numUnbalanced = 0;
        deleteBNodes(false);

        numBalanced = numberOfNodes;
        BalancedKDTreeNode[] dest = new BalancedKDTreeNode[numberOfNodes + 1];
        for (int i = 0; i < dest.length; i++) {
            dest[i] = new BalancedKDTreeNode();
        }
        dest[numberOfNodes].mData = new float[] {Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY};
        dest[numberOfNodes].mFlags = 64;

        balanceRec(broot, dest, 0, 0, numberOfNodes - 1);

        balancedRootNode = dest;
        firstLeaf = (numBalanced + 1) / 2;

        System.err.printf("done%n");
    }

    public void balanceRec(BalancedKDTreeNode[] broot, BalancedKDTreeNode[] dest, int destIndex, int low, int high) {
        if (low == high) {
            dest[destIndex] = broot[low];
            dest[destIndex].setDiscriminator(0);
            return;
        }

        int discr = bestDiscriminator(broot, low, high);
        int median = quickSelect(broot, low, high, discr);

        dest[destIndex] = broot[median];
        dest[destIndex].setDiscriminator(discr);

        if (low < median) {
            balanceRec(broot, dest, (destIndex << 1) + 1, low, median - 1);
        }
        if (high > median) {
            balanceRec(broot, dest, (destIndex << 1) + 2, median + 1, high);
        }
    }

    private Object assignData(Object data) {
        if (!copyData) {
            return data;
        }

        if (data instanceof float[]) {
            float[] p = (float[])data;
            return Arrays.copyOf(p, p.length);
        }

        if (data instanceof byte[]) {
            byte[] b = (byte[])data;
            return Arrays.copyOf(b, b.length);
        }

        return data;
    }

    private static float[] asPoint(Object data) {
        if (!(data instanceof float[])) {
            throw new IllegalArgumentException("KDTree data must be a float[] with xyz coordinates in first entries");
        }
        float[] point = (float[])data;
        if (point.length < 3) {
            throw new IllegalArgumentException("KDTree point data must have at least 3 components");
        }
        return point;
    }

    private void deleteNodes(KDTreeNode node, boolean deleteData) {
        if (node == null) {
            return;
        }

        deleteNodes(node.loson, deleteData);
        deleteNodes(node.hison, deleteData);

        node.loson = null;
        node.hison = null;
        if (deleteData) {
            node.mData = null;
        }
    }

    private void deleteBNodes(boolean deleteData) {
        if (balancedRootNode == null) {
            return;
        }

        if (deleteData) {
            for (int node = 0; node < numBalanced; node++) {
                balancedRootNode[node].mData = null;
            }
        }

        balancedRootNode = null;
    }

    private static float sqrDistance3D(float[] a, float[] b) {
        float tmp = a[0] - b[0];
        float result = tmp * tmp;

        tmp = a[1] - b[1];
        result += tmp * tmp;

        tmp = a[2] - b[2];
        result += tmp * tmp;

        return result;
    }

    private static void fixUp(KDQuery queryData) {
        int son = queryData.foundN;
        int parent = (son - 1) >> 1;

        while ((son > 0) && queryData.distances[parent] < queryData.distances[son]) {
            float tmpDist = queryData.distances[parent];
            Object tmpData = queryData.results[parent];

            queryData.distances[parent] = queryData.distances[son];
            queryData.results[parent] = queryData.results[son];

            queryData.distances[son] = tmpDist;
            queryData.results[son] = tmpData;

            son = parent;
            parent = (son - 1) >> 1;
        }
    }

    private static void mhInsert(KDQuery queryData, Object data, float dist) {
        queryData.distances[queryData.foundN] = dist;
        queryData.results[queryData.foundN] = data;

        fixUp(queryData);

        if (++queryData.foundN == queryData.wantedN) {
            queryData.maximumDistance = queryData.distances[0];
            queryData.notFilled = false;
        }
    }

    private static void fixDown(KDQuery queryData) {
        int max = queryData.foundN;
        int parent = 0;
        int son = 1;

        while (son < max) {
            if (queryData.distances[son] <= queryData.distances[parent]) {
                if ((++son >= max) || queryData.distances[son] <= queryData.distances[parent]) {
                    return;
                }
            }
            else if ((son + 1 < max) && queryData.distances[son + 1] > queryData.distances[son]) {
                son++;
            }

            float tmpDist = queryData.distances[parent];
            Object tmpData = queryData.results[parent];

            queryData.distances[parent] = queryData.distances[son];
            queryData.results[parent] = queryData.results[son];

            queryData.distances[son] = tmpDist;
            queryData.results[son] = tmpData;

            parent = son;
            son = (parent << 1) + 1;
        }
    }

    private static void mhReplaceMax(KDQuery queryData, Object data, float dist) {
        queryData.distances[0] = dist;
        queryData.results[0] = data;

        fixDown(queryData);
        queryData.maximumDistance = queryData.distances[0];
    }

    private void queryRec(KDTreeNode node, KDQuery queryData) {
        int discriminator = node.discriminator();
        float dist = sqrDistance3D(asPoint(node.mData), queryData.point);

        if (dist < queryData.maximumDistance) {
            if (queryData.notFilled) {
                mhInsert(queryData, node.mData, dist);
            }
            else {
                mhReplaceMax(queryData, node.mData, dist);
            }
        }

        dist = asPoint(node.mData)[discriminator] - queryData.point[discriminator];

        KDTreeNode nearNode;
        KDTreeNode farNode;
        if (dist >= 0.0f) {
            nearNode = node.loson;
            farNode = node.hison;
        }
        else {
            nearNode = node.hison;
            farNode = node.loson;
        }

        if (nearNode != null) {
            queryRec(nearNode, queryData);
        }

        dist *= dist;
        if (farNode != null && (((queryData.foundN < queryData.wantedN) &&
            (dist < queryData.sqrRadius)) ||
            (dist < queryData.maximumDistance))) {
            queryRec(farNode, queryData);
        }
    }

    private void balancedQueryRec(int index, KDQuery queryData) {
        BalancedKDTreeNode node = balancedRootNode[index];
        int discr = node.discriminator();
        float dist;
        int nearIndex;
        int farIndex;

        if (index < firstLeaf) {
            dist = asPoint(node.mData)[discr] - queryData.point[discr];

            if (dist >= 0.0f) {
                nearIndex = (index << 1) + 1;
                farIndex = nearIndex + 1;
            }
            else {
                farIndex = (index << 1) + 1;
                nearIndex = farIndex + 1;
            }

            if (nearIndex < numBalanced) {
                balancedQueryRec(nearIndex, queryData);
            }

            dist *= dist;
            if ((farIndex < numBalanced) && (((queryData.notFilled) &&
                (dist < queryData.sqrRadius)) ||
                (dist < queryData.maximumDistance))) {
                balancedQueryRec(farIndex, queryData);
            }
        }

        dist = sqrDistance3D(asPoint(node.mData), queryData.point);

        if (dist < queryData.maximumDistance) {
            if (queryData.notFilled) {
                mhInsert(queryData, node.mData, dist);
            }
            else {
                mhReplaceMax(queryData, node.mData, dist);
            }
        }
    }

    private static void bkdswap(BalancedKDTreeNode[] root, int a, int b) {
        BalancedKDTreeNode tmp = root[a];
        root[a] = root[b];
        root[b] = tmp;
    }

    private static float bkdval(BalancedKDTreeNode[] root, int index, int discr) {
        return asPoint(root[index].mData)[discr];
    }

    private static void eSwap(BalancedKDTreeNode[] broot, int a, int b) {
        bkdswap(broot, a, b);
    }

    private static float eVal(BalancedKDTreeNode[] broot, int index, int discr) {
        return bkdval(broot, index, discr);
    }

    private static int getBalancedMedian(int low, int high) {
        int n = high - low + 1;
        if (n <= 1) {
            return low;
        }

        int fl = (int)(Math.log(n + 0.1) / Math.log(2.0));
        int p2fl = (1 << fl);
        int lastN = n - (p2fl - 1);
        int lasts2 = p2fl / 2;

        int left;
        if (lastN < lasts2) {
            left = lastN + (lasts2 - 1);
        }
        else {
            left = lasts2 + lasts2 - 1;
        }

        return low + left;
    }

    private static int quickSelect(BalancedKDTreeNode[] broot, int low, int high, int discr) {
        int median = getBalancedMedian(low, high);

        while (true) {
            if (high <= low) {
                return median;
            }

            if (high == low + 1) {
                if (eVal(broot, low, discr) > eVal(broot, high, discr)) {
                    eSwap(broot, low, high);
                }
                return median;
            }

            int middle = (low + high + 1) / 2;
            if (eVal(broot, middle, discr) > eVal(broot, high, discr)) {
                eSwap(broot, middle, high);
            }
            if (eVal(broot, low, discr) > eVal(broot, high, discr)) {
                eSwap(broot, low, high);
            }
            if (eVal(broot, middle, discr) > eVal(broot, low, discr)) {
                eSwap(broot, middle, low);
            }

            eSwap(broot, middle, low + 1);

            int ll = low + 1;
            int hh = high;
            while (true) {
                do {
                    ll++;
                } while (eVal(broot, low, discr) > eVal(broot, ll, discr));

                do {
                    hh--;
                } while (eVal(broot, hh, discr) > eVal(broot, low, discr));

                if (hh < ll) {
                    break;
                }

                eSwap(broot, ll, hh);
            }

            eSwap(broot, low, hh);

            if (hh <= median) {
                low = ll;
            }
            if (hh >= median) {
                high = hh - 1;
            }
        }
    }

    private static void copyUnbalancedRec(KDTreeNode node, BalancedKDTreeNode[] broot, int[] pindex) {
        if (node != null) {
            broot[pindex[0]++].copy(node);
            copyUnbalancedRec(node.loson, broot, pindex);
            copyUnbalancedRec(node.hison, broot, pindex);
        }
    }

    private static int bestDiscriminator(BalancedKDTreeNode[] broot, int low, int high) {
        float[] bMin = new float[] {
            Numeric.HUGE_FLOAT_VALUE,
            Numeric.HUGE_FLOAT_VALUE,
            Numeric.HUGE_FLOAT_VALUE};
        float[] bMax = new float[] {
            -Numeric.HUGE_FLOAT_VALUE,
            -Numeric.HUGE_FLOAT_VALUE,
            -Numeric.HUGE_FLOAT_VALUE};

        for (int i = low; i <= high; i++) {
            float tmp = bkdval(broot, i, 0);
            if (bMin[0] > tmp) {
                bMin[0] = tmp;
            }
            if (bMax[0] < tmp) {
                bMax[0] = tmp;
            }

            tmp = bkdval(broot, i, 1);
            if (bMin[1] > tmp) {
                bMin[1] = tmp;
            }
            if (bMax[1] < tmp) {
                bMax[1] = tmp;
            }

            tmp = bkdval(broot, i, 2);
            if (bMin[2] > tmp) {
                bMin[2] = tmp;
            }
            if (bMax[2] < tmp) {
                bMax[2] = tmp;
            }
        }

        int discr = 0;
        float spread = bMax[0] - bMin[0];

        float tmp = bMax[1] - bMin[1];
        if (tmp > spread) {
            discr = 1;
            spread = tmp;
        }

        tmp = bMax[2] - bMin[2];
        if (tmp > spread) {
            discr = 2;
        }

        return discr;
    }
}

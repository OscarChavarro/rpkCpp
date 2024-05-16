#include <cstdio>
#include <cstring>

#include "java/lang/Math.h"
#include "common/error.h"
#include "common/linealAlgebra/Numeric.h"
#include "common/dataStructures/KDTree.h"

#define E_SWAP(a, b) bkdswap(broot, (a), (b))
#define E_VAL(index) bkdval(broot, (index), discr)

const float KD_MAX_RADIUS = 1e10;

// KD Tree with one data element per node
float *KDTree::s_distances = nullptr;

class KDQuery {
  public:
    float *point;
    int wantedN;
    int foundN;
    bool notFilled;
    float **results;
    float *distances;
    float maximumDistance;
    float sqrRadius;
    short excludeFlags;

    KDQuery():
        point(),
        wantedN(),
        foundN(),
        notFilled(),
        results(),
        distances(),
        maximumDistance(),
        sqrRadius(),
        excludeFlags()
    {}

    void print() const {
        printf("Point X %g, Y %g, Z %g\n", point[0], point[1], point[2]);
        printf("Wanted N: %i, found N: %i\n", wantedN, foundN);
        printf("maximumDistance %g\n", maximumDistance);
        printf("sqrRadius %g\n", sqrRadius);
        printf("excludeFlags %x\n", (int) excludeFlags);
    }
};

static KDQuery GLOBAL_qDatS;

KDTree::KDTree(int dataSize, bool CopyData) {
    m_dataSize = dataSize;
    m_numNodes = 0;

    m_numUnbalanced = 0;
    m_root = nullptr;

    m_numBalanced = 0;
    m_broot = nullptr;

    m_CopyData = CopyData;

    if ( s_distances == nullptr ) {
        s_distances = new float[1000];
    } // max 1000 !!

    m_firstLeaf = 0;
}

void
KDTree::deleteNodes(KDTreeNode *node, bool deleteData) {
    if ( node == nullptr ) {
        return;
    }

    deleteNodes(node->loson, deleteData);
    deleteNodes(node->hison, deleteData);

    if ( deleteData ) {
        free(node->m_data);
    }

    delete node;
}

void
KDTree::deleteBNodes(bool deleteData) {
    if ( m_broot == nullptr ) {
        return;
    }

    if ( deleteData ) {
        for ( int node = 0; node < m_numBalanced; node++ ) {
            free(m_broot[node].m_data);
        }
    }

    delete[] m_broot;
    m_broot = nullptr;
}

KDTree::~KDTree() {
    // Delete tree
    deleteNodes(m_root, m_CopyData);
    m_root = nullptr;
    deleteBNodes(m_CopyData);
}

/**
add a point in the kd tree, this is always to the unbalanced part
*/
void
KDTree::addPoint(void *data, short flags = 0) {
    // Add the point to the unbalanced part
    KDTreeNode **nodePtr;
    KDTreeNode *newNode;
    KDTreeNode *parent;
    const float *newPoint;
    int discriminator;

    newNode = new KDTreeNode;

    newNode->m_data = assignData(data);
    newNode->m_flags = flags;
    newNode->setDiscriminator(0); // Default

    newNode->loson = nullptr;
    newNode->hison = nullptr;

    newPoint = (float *) data;
    m_numNodes++;
    m_numUnbalanced++;

    nodePtr = &m_root;
    parent = nullptr;

    while ( *nodePtr != nullptr ) {
        parent = *nodePtr;  // The parent

        discriminator = parent->discriminator();

        // Test discriminator
        if ( newPoint[discriminator] <= ((float *) (parent->m_data))[discriminator] ) {
            nodePtr = &(parent->loson);
        } else {
            nodePtr = &(parent->hison);
        }
    }

    if ( (parent != nullptr) && (parent->loson == nullptr) && (parent->hison == nullptr) ) {
        // Choose an appropriate discriminator for the parent
        float dx;
        float dy;
        float dz;
        const float *customNodeData = (float *) (parent->m_data);

        dx = java::Math::abs(newPoint[0] - customNodeData[0]);
        dy = java::Math::abs(newPoint[1] - customNodeData[1]);
        dz = java::Math::abs(newPoint[2] - customNodeData[2]);

        if ( dx > dy ) {
            if ( dx > dz ) {
                discriminator = 0;
            } else {
                discriminator = 2;
            }
        } else {
            if ( dy > dz ) {
                discriminator = 1;
            } else {
                discriminator = 2;
            }
        }

        parent->setDiscriminator(discriminator);

        // Choose correct side
        if ( newPoint[discriminator] <= customNodeData[discriminator] ) {
            parent->loson = newNode;
        } else {
            parent->hison = newNode;
        }
    } else {
        // Parent is nullptr or discriminator is fixed...
        *nodePtr = newNode;
    }
}

void *
KDTree::assignData(void *data) const {
    if ( m_CopyData ) {
        void *newData;

        newData = malloc(m_dataSize);
        memcpy((char *) newData, (char *) data, m_dataSize);
        return newData;
    } else {
        return data;
    }
}

/**
Iterate nodes : iterate all nodes (only for balanced trees!)
*/
void
KDTree::iterateNodes(void (*callBack)(void *, void *), void *data) {
    if ( m_numUnbalanced > 0 ) {
        logError(" KDTree::iterateNodes", "Cannot iterate unbalanced trees");
        return;
    }

    for ( int i = 0; i < m_numBalanced; i++ ) {
        callBack(data, m_broot[i].m_data);
    }
}

/**
Query the kd tree : both balanced and unbalanced parts taken into
account ! (The balanced part is searched first)
*/
int
KDTree::query(
    const float *point,
    int N,
    void *results,
    float *distances,
    float radius,
    short excludeFlags)
{
    int numberFound;
    float *usedDistances;

    if ( distances == nullptr ) {
        if ( N > 1000 ) {
            logError("KDTree::query", "Too many nodes requested");
            return 0;
        }
        usedDistances = s_distances;
    } else {
        usedDistances = distances;
    }

    // Fill in static class data
    GLOBAL_qDatS.point = (float *)point;
    GLOBAL_qDatS.wantedN = N;
    GLOBAL_qDatS.foundN = 0;
    GLOBAL_qDatS.results = (float **) results;
    GLOBAL_qDatS.distances = usedDistances;
    GLOBAL_qDatS.maximumDistance = radius;
    GLOBAL_qDatS.sqrRadius = radius;
    GLOBAL_qDatS.excludeFlags = excludeFlags;
    GLOBAL_qDatS.notFilled = true;

    // First query balanced part
    if ( m_broot != nullptr ) {
        BQuery_rec(0);
    }

    // Now query unbalanced part using the already found nodes
    // from the balanced part
    if ( m_root ) {
        queryRec(m_root);
    }

    numberFound = GLOBAL_qDatS.foundN;

    return numberFound;
}

/**
Distance calculation
*/
inline static float
sqrDistance3D(const float *a, const float *b) {
    float result;
    float tmp;

    tmp = *(a++) - *(b++);
    result = tmp * tmp;

    tmp = *(a++) - *(b++);
    result += tmp * tmp;

    tmp = *a - *b;
    result += tmp * tmp;

    return result;
}

/**
Max heap stuff, using static GLOBAL_qDatS (fastest !!)
Adapted from patched POVRAY (megasrc), who took it from Sejwick
*/
inline static void
fixUp() {
    // Ripple the node (qdat_s.foundN) upward. There are qdat_s.foundN + 1 nodes
    // in the tree
    int son;
    int parent;
    float tmpDist;
    float *tmpData;

    son = GLOBAL_qDatS.foundN;
    parent = (son - 1) >> 1;  // Root of tree == index 0 so parent = any son - 1 / 2

    while ( (son > 0) && GLOBAL_qDatS.distances[parent] < GLOBAL_qDatS.distances[son] ) {
        tmpDist = GLOBAL_qDatS.distances[parent];
        tmpData = GLOBAL_qDatS.results[parent];

        GLOBAL_qDatS.distances[parent] = GLOBAL_qDatS.distances[son];
        GLOBAL_qDatS.results[parent] = GLOBAL_qDatS.results[son];

        GLOBAL_qDatS.distances[son] = tmpDist;
        GLOBAL_qDatS.results[son] = tmpData;

        son = parent;
        parent = (son - 1) >> 1;
    }
}

inline static void
mhInsert(float *data, float dist) {
    GLOBAL_qDatS.distances[GLOBAL_qDatS.foundN] = dist;
    GLOBAL_qDatS.results[GLOBAL_qDatS.foundN] = data;

    fixUp();

    // If all the photons are filled, we can use the actual maximum distance
    if ( ++GLOBAL_qDatS.foundN == GLOBAL_qDatS.wantedN ) {
        GLOBAL_qDatS.maximumDistance = GLOBAL_qDatS.distances[0];
        GLOBAL_qDatS.notFilled = false;
    }
}

inline static void
fixDown() {
    // Ripple the top node, which may not be max anymore downwards
    // There are qdat_s.foundN nodes in the tree, starting at index 0
    int son;
    int parent;
    float tmpDist;
    float *tmpData;

    int max = GLOBAL_qDatS.foundN;

    parent = 0;
    son = 1;

    while ( son < max ) {
        if ( GLOBAL_qDatS.distances[son] <= GLOBAL_qDatS.distances[parent] ) {
            if ((++son >= max) || GLOBAL_qDatS.distances[son] <= GLOBAL_qDatS.distances[parent] ) {
                return; // Node in place, left son and right son smaller
            }
        } else {
            if ((son + 1 < max) && GLOBAL_qDatS.distances[son + 1] > GLOBAL_qDatS.distances[son] ) {
                son++; // Take maximum of the two sons
            }
        }

        // Swap because son > parent
        tmpDist = GLOBAL_qDatS.distances[parent];
        tmpData = GLOBAL_qDatS.results[parent];

        GLOBAL_qDatS.distances[parent] = GLOBAL_qDatS.distances[son];
        GLOBAL_qDatS.results[parent] = GLOBAL_qDatS.results[son];

        GLOBAL_qDatS.distances[son] = tmpDist;
        GLOBAL_qDatS.results[son] = tmpData;

        parent = son;
        son = (parent << 1) + 1;
    }
}

inline static void
mhReplaceMax(float *data, float dist) {
    // Top = maximum element. Replace it with new and ripple down
    // The heap is full (foundN == wantedN), but this is not required

    *GLOBAL_qDatS.distances = dist; // Top
    *GLOBAL_qDatS.results = data;

    fixDown();

    GLOBAL_qDatS.maximumDistance = *GLOBAL_qDatS.distances; // Max = top of heap
}

/**
Query_rec for the unbalanced kd tree part
*/
void
KDTree::queryRec(const KDTreeNode *node) {
    int discriminator = node->discriminator();
    float dist;
    const KDTreeNode *nearNode;
    const KDTreeNode *farNode;

    dist = sqrDistance3D((float *) node->m_data, GLOBAL_qDatS.point);

    if ( dist < GLOBAL_qDatS.maximumDistance ) {
        if ( GLOBAL_qDatS.notFilled ) {
            // Add this point anyway, because we haven't got enough positions yet.
            // We have to check for the radius only here, since if N positions
            // are added, maximumDistance <= radius
            mhInsert((float *) node->m_data, dist);
        } else {
            // Add point if distance < maximumDistance
            mhReplaceMax((float *) node->m_data, dist);
        }
    }

    // Reuse distance
    dist = ((float *) node->m_data)[discriminator] - GLOBAL_qDatS.point[discriminator];

    if ( dist >= 0.0 ) {
        nearNode = node->loson;
        farNode = node->hison;
    } else {
        nearNode = node->hison;
        farNode = node->loson;
    }

    // Always call near node recursively
    if ( nearNode ) {
        queryRec(nearNode);
    }

    dist *= dist; // Square distance to the separator plane
    if ( farNode && (((GLOBAL_qDatS.foundN < GLOBAL_qDatS.wantedN) &&
                        (dist < GLOBAL_qDatS.sqrRadius)) ||
                       (dist < GLOBAL_qDatS.maximumDistance)) ) {
        // Discriminator line closer than maximumDistance : nearer positions can lie
        // on the far side. Or there are still not enough nodes found
        queryRec(farNode);
    }
}

/**
Query_rec for the unbalanced kd tree part
*/
void
KDTree::BQuery_rec(int index) {
    const BalancedKDTreeNode &node = m_broot[index];
    int discr = node.discriminator();
    float dist;
    int nearIndex;
    int farIndex;

    // Recursive call to the child nodes

    // Test discr (reuse distance)
    if ( index < m_firstLeaf ) {
        dist = ((float *) node.m_data)[discr] - GLOBAL_qDatS.point[discr];

        if ( dist >= 0.0 ) {
            nearIndex = (index << 1) + 1; // node loson
            farIndex = nearIndex + 1; // node hison
        } else {
            farIndex = (index << 1) + 1; // node loson
            nearIndex = farIndex + 1; // node hison
        }

        // Always call near node recursively
        if ( nearIndex < m_numBalanced ) {
            BQuery_rec(nearIndex);
        }

        dist *= dist; // Square distance to the separator plane
        if ((farIndex < m_numBalanced) && (((GLOBAL_qDatS.notFilled) && // qdat_s.foundN < qdat_s.wantedN
                                            (dist < GLOBAL_qDatS.sqrRadius)) ||
                                           (dist < GLOBAL_qDatS.maximumDistance)) ) {
            // Discriminator line closer than maximumDistance : nearer positions can lie
            // on the far side. Or there are still not enough nodes found
            BQuery_rec(farIndex);
        }
    }

    dist = sqrDistance3D((float *)node.m_data, GLOBAL_qDatS.point);

    if ( dist < GLOBAL_qDatS.maximumDistance ) {
        if ( GLOBAL_qDatS.notFilled ) {
            // Add this point anyway, because we haven't got enough positions yet.
            // We have to check for the radius only here, since if N positions
            // are added, maximumDistance <= radius
            mhInsert((float *) node.m_data, dist);
        } else {
            // Add point if distance < maximumDistance
            mhReplaceMax((float *) node.m_data, dist);
        }
    }
}

void
KDTreeNode::findMinMaxDepth(int depth, int *minDepth, int *maxDepth) const {
    if ( (loson == nullptr) && (hison == nullptr) ) {
        *maxDepth = java::Math::max(*maxDepth, depth);
        *minDepth = java::Math::min(*minDepth, depth);
    } else {
        if ( loson ) {
            loson->findMinMaxDepth(depth + 1, minDepth, maxDepth);
        }
        if ( hison ) {
            hison->findMinMaxDepth(depth + 1, minDepth, maxDepth);
        }
    }
}

/**
This Quick select routine is adapted from the algorithm described in
"Numerical recipes in C", Second Edition,
Cambridge University Press, 1992, Section 8.5, ISBN 0-521-43108-5
*/
static inline void
bkdswap(BalancedKDTreeNode root[], int a, int b) {
    BalancedKDTreeNode tmp = root[a];
    root[a] = root[b];
    root[b] = tmp;
}

static inline float
bkdval(BalancedKDTreeNode root[], int index, int discr) {
    return ((float *)root[index].m_data)[discr];
}

int
getBalancedMedian(int low, int high) {
    int N = high - low + 1;  // High inclusive

    if ( N <= 1 ) {
        return low;
    }

    int FL;
    // Add 0.1 because integer powers of 2 sometimes gave a smaller FL (8 -> 2)
    FL = (int) (java::Math::log(N + 0.1) / M_LN2);

    int P2FL = (1 << FL); // 2^FL
    int LASTN = N - (P2FL - 1);  // Number of elements on last level
    int LASTS_2 = P2FL / 2; // Half the room for elements on last level
    int left;

    if ( LASTN < LASTS_2 ) {
        // All in left subtree
        left = LASTN + (LASTS_2 - 1);
    } else {
        // Full bottom level in left subtree
        left = LASTS_2 + LASTS_2 - 1;
        // Rest in right subtree
    }

    return low + left;
}

/**
Return index of median element
*/
static int
quickSelect(BalancedKDTreeNode broot[], int low, int high, int discr) {
    int median;
    int middle;
    int ll;
    int hh;

    median = getBalancedMedian(low, high);

    for ( ;; ) {
        if ( high <= low ) {
            // One element only
            return median;
        }

        if ( high == low + 1 ) {
            // Two elements only
            if ( E_VAL(low) > E_VAL(high) ) {
                E_SWAP(low, high);
            }
            return median;
        }

        // Find median of low, middle and high volumeListsOfItems; swap into position low
        middle = (low + high + 1) / 2;
        if ( E_VAL(middle) > E_VAL(high) ) {
            E_SWAP(middle, high);
        }
        if ( E_VAL(low) > E_VAL(high) ) {
            E_SWAP(low, high);
        }
        if ( E_VAL(middle) > E_VAL(low) ) {
            E_SWAP(middle, low);
        }

        // Swap low item (now in position middle) into position (low + 1)
        E_SWAP(middle, low + 1);

        // Nibble from each end towards middle, swapping volumeListsOfItems when stuck
        ll = low + 1;
        hh = high;
        for ( ;; ) {
            do {
                ll++;
            } while ( E_VAL(low) > E_VAL(ll));
            do {
                hh--;
            } while ( E_VAL(hh) > E_VAL(low));

            if ( hh < ll ) {
                break;
            }

            E_SWAP(ll, hh);
        }

        // Swap middle item (in position low) back into correct position
        E_SWAP(low, hh);

        // Re-set active partition
        if ( hh <= median ) {
            low = ll;
        }
        if ( hh >= median ) {
            high = hh - 1;
        }
    }
}

/**
Balance the kd tree
*/
static void
copyUnbalancedRec(KDTreeNode *node, BalancedKDTreeNode *broot, int *pindex) {
    if ( node ) {
        broot[(*pindex)++].copy(*node);
        copyUnbalancedRec(node->loson, broot, pindex);
        copyUnbalancedRec(node->hison, broot, pindex);
    }
}

static int
bestDiscriminator(BalancedKDTreeNode broot[], int low, int high) {
    float bMin[3] = {Numeric::HUGE_FLOAT_VALUE, Numeric::HUGE_FLOAT_VALUE, Numeric::HUGE_FLOAT_VALUE};
    float bMax[3] = {-Numeric::HUGE_FLOAT_VALUE, -Numeric::HUGE_FLOAT_VALUE, -Numeric::HUGE_FLOAT_VALUE};
    float tmp;

    for ( int i = low; i <= high; i++ ) {
        tmp = bkdval(broot, i, 0);
        if ( bMin[0] > tmp ) {
            bMin[0] = tmp;
        }
        if ( bMax[0] < tmp ) {
            bMax[0] = tmp;
        }

        tmp = bkdval(broot, i, 1);
        if ( bMin[1] > tmp ) {
            bMin[1] = tmp;
        }
        if ( bMax[1] < tmp ) {
            bMax[1] = tmp;
        }

        tmp = bkdval(broot, i, 2);
        if ( bMin[2] > tmp ) {
            bMin[2] = tmp;
        }
        if ( bMax[2] < tmp ) {
            bMax[2] = tmp;
        }
    }

    int discr = 0;
    float spread = bMax[0] - bMin[0]; // X spread

    tmp = bMax[1] - bMin[1];
    if ( tmp > spread ) {
        discr = 1;
        spread = tmp;
    }

    tmp = bMax[2] - bMin[2];
    if ( tmp > spread ) {
        discr = 2;
    }

    return discr;
}

/**
balance the tree recursively
*/
void
KDTree::balanceRec(
    BalancedKDTreeNode broot[],
    BalancedKDTreeNode dest[],
    int destIndex,
    int low,
    int high) // High inclusive!
{
    if ( low == high ) {
        // Put it in dest
        dest[destIndex] = broot[low];
        dest[destIndex].setDiscriminator(0); // Don't care
        return;
    }

    int discr = bestDiscriminator(broot, low, high);
    // Find the balance median element
    int median = quickSelect(broot, low, high, discr);

    // Put it in dest
    dest[destIndex] = broot[median];
    dest[destIndex].setDiscriminator(discr);

    // Recurse low and high array
    if ( low < median ) {
        // High inclusive!
        balanceRec(broot, dest, (destIndex << 1) + 1, low, median - 1);
    }
    if ( high > median )
        balanceRec(broot, dest, (destIndex << 1) + 2, median + 1, high);  // High inclusive!
}

/**
Balance the tree, it is possible that a part is already balanced!
*/
void
KDTree::balance() {
    // Make an unsorted BalancedKDTreeNode array pointing to the nodes
    if ( m_numUnbalanced == 0 ) {
        // No balancing needed.
        return;
    }

    fprintf(stderr, "Balancing kd-tree: %i nodes...\n", m_numNodes);

    BalancedKDTreeNode *broot = new BalancedKDTreeNode[m_numNodes + 1];

    broot[m_numNodes].m_data = nullptr;
    broot[m_numNodes].m_flags = 128;

    int index = 0;

    // Copy balanced
    for ( int i = 0; i < m_numBalanced; i++ ) {
        broot[index++] = m_broot[i];
    }

    // Copy unbalanced
    copyUnbalancedRec(m_root, broot, &index);

    // Clear old balanced and unbalanced part (but no data delete)
    deleteNodes(m_root, false);
    m_root = nullptr;
    m_numUnbalanced = 0;

    deleteBNodes(false);

    m_numBalanced = m_numNodes;
    BalancedKDTreeNode *dest = new BalancedKDTreeNode[m_numNodes + 1]; // Could we do with just 1 array???

    dest[m_numNodes].m_data = nullptr;
    dest[m_numNodes].m_flags = 64;

    // Now balance the tree recursively
    balanceRec(broot, dest, 0, 0, m_numNodes - 1);  // High inclusive!

    m_broot = dest;
    delete[] broot;

    m_firstLeaf = (m_numBalanced + 1) / 2;

    fprintf(stderr, "done\n");
}

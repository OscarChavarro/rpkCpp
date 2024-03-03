#include <cstdio>
#include <cstring>

#include "common/dataStructures/KDTree.h"
#include "common/error.h"

#define E_SWAP(a, b) bkdswap(broot, (a), (b))
#define E_VAL(index) bkdval(broot, (index), discr)

// KD Tree with one data element per node
float *KDTree::s_distances = nullptr;

class Ckdquery {
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

    void print() const {
        printf("Point X %g, Y %g, Z %g\n", point[0], point[1], point[2]);
        printf("Wanted N: %i, found N: %i\n", wantedN, foundN);
        printf("maximumDistance %g\n", maximumDistance);
        printf("sqrRadius %g\n", sqrRadius);
        printf("excludeFlags %x\n", (int) excludeFlags);
    }

};

static Ckdquery GLOBAL_qdatS;

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

void
KDTree::addPoint(void *data, short flags = 0) {
    // Add the point to the unbalanced part
    KDTreeNode **nodePtr, *newNode, *parent;
    float *newPoint;
    int discriminator;

    newNode = new KDTreeNode;

    newNode->m_data = AssignData(data);
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

    if ( (parent != nullptr) && (parent->loson == nullptr) && (parent->hison == nullptr)) {
        // Choose an appropriate discriminator for the parent
        float dx, dy, dz;
        float *pdata = (float *) (parent->m_data);

        dx = std::fabs(newPoint[0] - pdata[0]);
        dy = std::fabs(newPoint[1] - pdata[1]);
        dz = std::fabs(newPoint[2] - pdata[2]);

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
        if ( newPoint[discriminator] <= pdata[discriminator] ) {
            parent->loson = newNode;
        } else {
            parent->hison = newNode;
        }
    } else {
        // Parent is nullptr or discriminator is fixed...
        *nodePtr = newNode;
    }
}

int
KDTree::Query(
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
            logError("KDTree::Query", "Too many nodes requested");
            return 0;
        }
        usedDistances = s_distances;
    } else {
        usedDistances = distances;
    }

    // Fill in static class data
    GLOBAL_qdatS.point = (float *) point;
    GLOBAL_qdatS.wantedN = N;
    GLOBAL_qdatS.foundN = 0;
    GLOBAL_qdatS.results = (float **) results;
    GLOBAL_qdatS.distances = usedDistances;
    GLOBAL_qdatS.maximumDistance = radius;
    GLOBAL_qdatS.sqrRadius = radius;
    GLOBAL_qdatS.excludeFlags = excludeFlags;
    GLOBAL_qdatS.notFilled = true;

    // First query balanced part
    if ( m_broot ) {
        BQuery_rec(0);
    }

    // Now query unbalanced part using the already found nodes
    // from the balanced part
    if ( m_root ) {
        Query_rec(m_root);
    }

    numberFound = GLOBAL_qdatS.foundN;

    return (numberFound);
}

void *
KDTree::AssignData(void *data) const {
    if ( m_CopyData ) {
        void *newData;

        newData = malloc(m_dataSize);
        memcpy((char *) newData, (char *) data, m_dataSize);
        return newData;
    } else {
        return data;
    }
}

// Distance calculation
inline static float
SqrDistance3D(float *a, float *b) {
    float result, tmp;

    tmp = *(a++) - *(b++);
    result = tmp * tmp;
    //  result = *ptmp++

    tmp = *(a++) - *(b++);
    result += tmp * tmp;

    tmp = *a - *b;
    result += tmp * tmp;

    return result;
}

// Max heap stuff, using static qdat_s  (fastest !!)
// Adapted from patched POVRAY (megasrc), who took it from Sejwick
inline static void
fixUp() {
    // Ripple the node (qdat_s.foundN) upward. There are qdat_s.foundN + 1 nodes
    // in the tree

    int son, parent;
    float tmpDist;
    float *tmpData;

    son = GLOBAL_qdatS.foundN;
    parent = (son - 1) >> 1;  // Root of tree == index 0 so parent = any son - 1 / 2

    while ((son > 0) && GLOBAL_qdatS.distances[parent] < GLOBAL_qdatS.distances[son] ) {
        tmpDist = GLOBAL_qdatS.distances[parent];
        tmpData = GLOBAL_qdatS.results[parent];

        GLOBAL_qdatS.distances[parent] = GLOBAL_qdatS.distances[son];
        GLOBAL_qdatS.results[parent] = GLOBAL_qdatS.results[son];

        GLOBAL_qdatS.distances[son] = tmpDist;
        GLOBAL_qdatS.results[son] = tmpData;

        son = parent;
        parent = (son - 1) >> 1;
    }
}

inline static void
MHInsert(float *data, float dist) {
    GLOBAL_qdatS.distances[GLOBAL_qdatS.foundN] = dist;
    GLOBAL_qdatS.results[GLOBAL_qdatS.foundN] = data;

    fixUp();

    // If all the photons are filled, we can use the actual maximum distance
    if ( ++GLOBAL_qdatS.foundN == GLOBAL_qdatS.wantedN ) {
        GLOBAL_qdatS.maximumDistance = GLOBAL_qdatS.distances[0];
        GLOBAL_qdatS.notFilled = false;
    }
}

inline static void
fixDown() {
    // Ripple the top node, which may not be max anymore downwards
    // There are qdat_s.foundN nodes in the tree, starting at index 0

    int son, parent, max;
    float tmpDist;
    float *tmpData;

    max = GLOBAL_qdatS.foundN;

    parent = 0;
    son = 1;

    while ( son < max ) {
        if ( GLOBAL_qdatS.distances[son] <= GLOBAL_qdatS.distances[parent] ) {
            if ((++son >= max) || GLOBAL_qdatS.distances[son] <= GLOBAL_qdatS.distances[parent] ) {
                return; // Node in place, left son and right son smaller
            }
        } else {
            if ((son + 1 < max) && GLOBAL_qdatS.distances[son + 1] > GLOBAL_qdatS.distances[son] ) {
                son++; // Take maximum of the two sons
            }
        }

        // Swap because son > parent

        tmpDist = GLOBAL_qdatS.distances[parent];
        tmpData = GLOBAL_qdatS.results[parent];

        GLOBAL_qdatS.distances[parent] = GLOBAL_qdatS.distances[son];
        GLOBAL_qdatS.results[parent] = GLOBAL_qdatS.results[son];

        GLOBAL_qdatS.distances[son] = tmpDist;
        GLOBAL_qdatS.results[son] = tmpData;

        parent = son;
        son = (parent << 1) + 1;
    }
}

inline static void
MHReplaceMax(float *data, float dist) {
    // Top = maximum element. Replace it with new and ripple down
    // The heap is full (foundN == wantedN), but this is not required

    *GLOBAL_qdatS.distances = dist; // Top
    *GLOBAL_qdatS.results = data;

    fixDown();

    GLOBAL_qdatS.maximumDistance = *GLOBAL_qdatS.distances; // Max = top of heap
}

// Query_rec for the unbalanced kdtree part
void
KDTree::Query_rec(const KDTreeNode *node) {
    //if(!node)
    //  return;

    int discr = node->discriminator();

    float dist;
    KDTreeNode *nearNode, *farNode;

    // if(!(node->m_flags & qdat_s.excludeFlags))
    {
        dist = SqrDistance3D((float *) node->m_data, GLOBAL_qdatS.point);

        if ( dist < GLOBAL_qdatS.maximumDistance ) {
            if ( GLOBAL_qdatS.notFilled ) {
                // Add this point anyway, because we haven't got enough positions yet.
                // We have to check for the radius only here, since if N positions
                // are added, maximumDistance <= radius
                MHInsert((float *) node->m_data, dist);
            } else {
                // Add point if distance < maximumDistance
                MHReplaceMax((float *) node->m_data, dist);
            }
        }
    }

    // Reuse dist
    dist = ((float *) node->m_data)[discr] - GLOBAL_qdatS.point[discr];

    if ( dist >= 0.0 ) {
        nearNode = node->loson;
        farNode = node->hison;
    } else {
        nearNode = node->hison;
        farNode = node->loson;
    }

    // Always call near node recursively
    if ( nearNode ) {
        Query_rec(nearNode);
    }

    dist *= dist; // Square distance to the separator plane
    if ((farNode) && (((GLOBAL_qdatS.foundN < GLOBAL_qdatS.wantedN) &&
                       (dist < GLOBAL_qdatS.sqrRadius)) ||
                      (dist < GLOBAL_qdatS.maximumDistance))) {
        // Discriminator line closer than maxdist : nearer positions can lie
        // on the far side. Or there are still not enough nodes found

        Query_rec(farNode);
    }
}

// Query_rec for the unbalanced kd tree part
void
KDTree::BQuery_rec(int index) {
    const BalancedKDTreeNode &node = m_broot[index];
    int discr = node.Discriminator();

    float dist;

    int nearIndex, farIndex;

    // Recursive call to the child nodes

    // Test discr (reuse dist)
    if ( index < m_firstLeaf ) {
        dist = ((float *) node.m_data)[discr] - GLOBAL_qdatS.point[discr];

        if ( dist >= 0.0 )  // qdat_s.point[discr] <= ((float *)node->m_data)[discr]
        {
            nearIndex = (index << 1) + 1; //node->loson;
            farIndex = nearIndex + 1; //node->hison;
        } else {
            farIndex = (index << 1) + 1;//node->loson;
            nearIndex = farIndex + 1;//node->hison;
        }

        // Always call near node recursively

        if ( nearIndex < m_numBalanced ) {
            BQuery_rec(nearIndex);
        }

        dist *= dist; // Square distance to the separator plane
        if ((farIndex < m_numBalanced) && (((GLOBAL_qdatS.notFilled) && // qdat_s.foundN < qdat_s.wantedN
                                            (dist < GLOBAL_qdatS.sqrRadius)) ||
                                           (dist < GLOBAL_qdatS.maximumDistance))) {
            // Discriminator line closer than maximumDistance : nearer positions can lie
            // on the far side. Or there are still not enough nodes found
            BQuery_rec(farIndex);
        }
    }

    dist = SqrDistance3D((float *) node.m_data, GLOBAL_qdatS.point);

    if ( dist < GLOBAL_qdatS.maximumDistance ) {
        if ( GLOBAL_qdatS.notFilled ) // foundN < qdat_s.wantedN)
        {
            // Add this point anyway, because we haven't got enough positions yet.
            // We have to check for the radius only here, since if N positions
            // are added, maximumDistance <= radius

            MHInsert((float *) node.m_data, dist);
        } else {
            // Add point if distance < maximumDistance
            MHReplaceMax((float *) node.m_data, dist);
        }
    }
}

void
KDTreeNode::findMinMaxDepth(int depth, int *minDepth, int *maxDepth) const {
    if ( (loson == nullptr) && (hison == nullptr) ) {
        *maxDepth = intMax(*maxDepth, depth);
        *minDepth = intMin(*minDepth, depth);
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
    return (((float *) root[index].m_data)[discr]);
}

int
GetBalancedMedian(int low, int high) {
    int N = high - low + 1;  // high inclusive

    if ( N <= 1 ) {
        return low;
    }

    int FL;
    // Add 0.1 because integer powers of 2 sometimes gave a smaller FL (8 -> 2)
    FL = (int) (std::log(N + 0.1) / M_LN2); // frexp((double)N, &FL);
    //FL++; // Filled levels

    int P2FL = (1 << FL); // 2^FL
    int LASTN = N - (P2FL - 1);  // Number of elements on last level
    int LASTS_2 = P2FL / 2; // Half the room for elements on last level
    int left;

    if ( LASTN < LASTS_2 ) {
        // All in left subtree
        left = LASTN + (LASTS_2 - 1);
        //right = LASTS_2 - 1;
    } else {
        // Full bottom level in left subtree
        left = LASTS_2 + LASTS_2 - 1;
        // Rest in right subtree
        //right = LASTN - LASTS_2 + LASTS_2 - 1;
    }

    return low + left; //+1;
}

// quick_select: return index of median element
static int
quick_select(BalancedKDTreeNode broot[], int low, int high, int discr) {
    // int low, high;
    int median;
    int middle;
    int ll;
    int hh;

    // low = 0 ; high = n-1 ;
    // median = (low + high + 1) / 2;

    median = GetBalancedMedian(low, high);

    for ( ;; ) {
        if ( high <= low ) /* One element only */
        {
            // fprintf(stderr, "Median %i\n", median);
            return median;
        }

        if ( high == low + 1 ) {
            // Two elements only
            if ( E_VAL(low) > E_VAL(high))
                E_SWAP(low, high);
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

        /* Nibble from each end towards middle, swapping volumeListsOfItems when stuck */
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
Balance the kdtree
*/
static void
CopyUnbalanced_rec(KDTreeNode *node, BalancedKDTreeNode *broot, int *pindex) {
    if ( node ) {
        broot[(*pindex)++].Copy(*node);
        CopyUnbalanced_rec(node->loson, broot, pindex);
        CopyUnbalanced_rec(node->hison, broot, pindex);
    }
}

static int
BestDiscriminator(BalancedKDTreeNode broot[], int low, int high) {
    float bmin[3] = {HUGE, HUGE, HUGE};
    float bmax[3] = {-HUGE, -HUGE, -HUGE};
    float tmp;

    for ( int i = low; i <= high; i++ ) {
        tmp = bkdval(broot, i, 0);
        if ( bmin[0] > tmp ) {
            bmin[0] = tmp;
        }
        if ( bmax[0] < tmp ) {
            bmax[0] = tmp;
        }

        tmp = bkdval(broot, i, 1);
        if ( bmin[1] > tmp ) {
            bmin[1] = tmp;
        }
        if ( bmax[1] < tmp ) {
            bmax[1] = tmp;
        }

        tmp = bkdval(broot, i, 2);
        if ( bmin[2] > tmp ) {
            bmin[2] = tmp;
        }
        if ( bmax[2] < tmp ) {
            bmax[2] = tmp;
        }
    }

    int discr = 0;
    float spread = bmax[0] - bmin[0]; // X spread

    tmp = bmax[1] - bmin[1];
    if ( tmp > spread ) {
        discr = 1;
        spread = tmp;
    }

    tmp = bmax[2] - bmin[2];
    if ( tmp > spread ) {
        discr = 2;
    }

    return discr;
}

/**
Balance the tree recursively
*/
void
KDTree::Balance_rec(
    BalancedKDTreeNode broot[],
    BalancedKDTreeNode dest[],
    int destIndex,
    int low,
    int high) // High inclusive!
{
    if ( low == high ) {
        // Put it in dest
        dest[destIndex] = broot[low];
        dest[destIndex].SetDiscriminator(0); // don't care...
        return;
    }

    int discr = BestDiscriminator(broot, low, high);
    // Find the balance median element
    int median = quick_select(broot, low, high, discr);

    // Put it in dest
    dest[destIndex] = broot[median];
    dest[destIndex].SetDiscriminator(discr);

    // Recurse low and high array

    if ( low < median ) {
        Balance_rec(broot, dest, (destIndex << 1) + 1, low, median - 1);
    }  // High inclusive!
    if ( high > median )
        Balance_rec(broot, dest, (destIndex << 1) + 2, median + 1, high);  // High inclusive!
}

void
KDTree::Balance() {
    // Make an unsorted BalancedKDTreeNode array pointing to the nodes
    if ( m_numUnbalanced == 0 ) {
        return;
    } // No balancing needed.

    fprintf(stderr, "Balancing kd-tree: %i nodes...\n", m_numNodes);

    BalancedKDTreeNode *broot = new BalancedKDTreeNode[m_numNodes + 1];

    broot[m_numNodes].m_data = nullptr;
    broot[m_numNodes].m_flags = 128;

    int i, index = 0;

    // Copy balanced
    for ( i = 0; i < m_numBalanced; i++ ) {
        broot[index++] = m_broot[i];
    }

    // Copy unbalanced
    CopyUnbalanced_rec(m_root, broot, &index);

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
    Balance_rec(broot, dest, 0, 0, m_numNodes - 1);  // High inclusive!

    m_broot = dest;
    delete[] broot;

    m_firstLeaf = (m_numBalanced + 1) / 2;

    fprintf(stderr, "done\n");
}

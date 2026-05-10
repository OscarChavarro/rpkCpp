#include <string.h>

#include "java/lang/Math.h"
#include "java/lang/System.h"
#include "common/linealAlgebra/Numeric.h"
#include "common/logging/Logger.h"
#include "common/dataStructures/KDQuery.h"
#include "common/dataStructures/KDTree.h"

// KD Tree with one data element per node
float *KDTree::distances = NULL;

KDTree::KDTree(int inDataSize, bool CopyData) {
    dataSize = inDataSize;
    numberOfNodes = 0;

    numUnbalanced = 0;
    root = NULL;

    numBalanced = 0;
    balancedRootNode = NULL;

    copyData = CopyData;

    if ( distances == NULL ) {
        // Maximum 1000!
        distances = new float[1000];
    }
    firstLeaf = 0;
}

void
KDTree::deleteNodes(KDTreeNode *node, bool deleteData) {
    if ( node == NULL ) {
        return;
    }

    deleteNodes(node->loson, deleteData);
    deleteNodes(node->hison, deleteData);

    if ( deleteData ) {
        delete[] ((unsigned char *)(node->m_data));
    }

    delete node;
}

void
KDTree::deleteBNodes(bool deleteData) {
    if ( balancedRootNode == NULL ) {
        return;
    }

    if ( deleteData ) {
        for ( int node = 0; node < numBalanced; node++ ) {
            delete[] ((unsigned char *)(balancedRootNode[node].m_data));
        }
    }

    delete[] balancedRootNode;
    balancedRootNode = NULL;
}

KDTree::~KDTree() {
    deleteNodes(root, copyData);
    root = NULL;
    deleteBNodes(copyData);
}

/**
Add a point in the kd tree, this is always to the unbalanced part
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

    newNode->loson = NULL;
    newNode->hison = NULL;

    newPoint = ((float *)(data));
    numberOfNodes++;
    numUnbalanced++;

    nodePtr = &root;
    parent = NULL;

    while ( *nodePtr != NULL ) {
        parent = *nodePtr; // The parent

        discriminator = parent->discriminator();

        // Test discriminator
        if ( newPoint[discriminator] <= ((float *)(parent->m_data))[discriminator] ) {
            nodePtr = &(parent->loson);
        } else {
            nodePtr = &(parent->hison);
        }
    }

    if ( (parent != NULL) && (parent->loson == NULL) && (parent->hison == NULL) ) {
        // Choose an appropriate discriminator for the parent
        float dx;
        float dy;
        float dz;
        const float *customNodeData = ((float *)(parent->m_data));

        dx = Math::abs(newPoint[0] - customNodeData[0]);
        dy = Math::abs(newPoint[1] - customNodeData[1]);
        dz = Math::abs(newPoint[2] - customNodeData[2]);

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
        // Parent is NULL or discriminator is fixed...
        *nodePtr = newNode;
    }
}

void *
KDTree::assignData(void *data) const {
    if ( copyData ) {
        unsigned char *newData = new unsigned char[((size_t)(dataSize))];
        memcpy(newData, data, ((size_t)(dataSize)));
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
    if ( numUnbalanced > 0 ) {
        Logger::error(" KDTree::iterateNodes", "Cannot iterate unbalanced trees");
        return;
    }

    for ( int i = 0; i < numBalanced; i++ ) {
        callBack(data, balancedRootNode[i].m_data);
    }
}

/**
Query the kd tree : both balanced and unbalanced parts taken into
account ! (The balanced part is searched first)
*/
int
KDTree::query(
    float *point,
    int N,
    void *results,
    float *inDistances,
    float radius,
    short excludeFlags)
{
    KDQuery queryData;
    int numberFound;
    float *usedDistances;

    if ( inDistances == NULL ) {
        if ( N > 1000 ) {
            Logger::error("KDTree::query", "Too many nodes requested");
            return 0;
        }
        usedDistances = inDistances;
    } else {
        usedDistances = inDistances;
    }

    // Fill in static class data
    queryData.point = point;
    queryData.wantedN = N;
    queryData.foundN = 0;
    queryData.results = ((float **)(results));
    queryData.distances = usedDistances;
    queryData.maximumDistance = radius;
    queryData.sqrRadius = radius;
    queryData.excludeFlags = excludeFlags;
    queryData.notFilled = true;

    // First query balanced part
    if ( balancedRootNode != NULL ) {
        balancedQueryRec(0, queryData);
    }

    // Now query unbalanced part using the already found nodes
    // from the balanced part
    if ( root ) {
        queryRec(root, queryData);
    }

    numberFound = queryData.foundN;

    return numberFound;
}

/**
Distance calculation
*/
inline float
KDTree::sqrDistance3D(const float *a, const float *b) {
    float result;
    float tmp;

    tmp = a[0] - b[0];
    result = tmp * tmp;

    tmp = a[1] - b[1];
    result += tmp * tmp;

    tmp = a[2] - b[2];
    result += tmp * tmp;

    return result;
}

/**
Max heap stuff
Adapted from patched POVRAY (megasrc), who took it from Sejwick
*/
inline void
KDTree::fixUp(KDQuery &queryData) {
    // Ripple the node (qdat_s.foundN) upward. There are qdat_s.foundN + 1 nodes
    // in the tree
    int son;
    int parent;
    float tmpDist;
    float *tmpData;

    son = queryData.foundN;
    parent = (son - 1) >> 1;  // Root of tree == index 0 so parent = any son - 1 / 2

    while ( (son > 0) && queryData.distances[parent] < queryData.distances[son] ) {
        tmpDist = queryData.distances[parent];
        tmpData = queryData.results[parent];

        queryData.distances[parent] = queryData.distances[son];
        queryData.results[parent] = queryData.results[son];

        queryData.distances[son] = tmpDist;
        queryData.results[son] = tmpData;

        son = parent;
        parent = (son - 1) >> 1;
    }
}

inline void
KDTree::mhInsert(KDQuery &queryData, float *data, float dist) {
    queryData.distances[queryData.foundN] = dist;
    queryData.results[queryData.foundN] = data;

    KDTree::fixUp(queryData);

    // If all the photons are filled, we can use the actual maximum distance
    if ( ++queryData.foundN == queryData.wantedN ) {
        queryData.maximumDistance = queryData.distances[0];
        queryData.notFilled = false;
    }
}

inline void
KDTree::fixDown(KDQuery &queryData) {
    // Ripple the top node, which may not be max anymore downwards
    // There are qdat_s.foundN nodes in the tree, starting at index 0
    int son;
    int parent;
    float tmpDist;
    float *tmpData;

    int max = queryData.foundN;

    parent = 0;
    son = 1;

    while ( son < max ) {
        if ( queryData.distances[son] <= queryData.distances[parent] ) {
            if ((++son >= max) || queryData.distances[son] <= queryData.distances[parent] ) {
                return; // Node in place, left son and right son smaller
            }
        } else {
            if ((son + 1 < max) && queryData.distances[son + 1] > queryData.distances[son] ) {
                son++; // Take maximum of the two sons
            }
        }

        // Swap because son > parent
        tmpDist = queryData.distances[parent];
        tmpData = queryData.results[parent];

        queryData.distances[parent] = queryData.distances[son];
        queryData.results[parent] = queryData.results[son];

        queryData.distances[son] = tmpDist;
        queryData.results[son] = tmpData;

        parent = son;
        son = (parent << 1) + 1;
    }
}

inline void
KDTree::mhReplaceMax(KDQuery &queryData, float *data, float dist) {
    // Top = maximum element. Replace it with new and ripple down
    // The heap is full (foundN == wantedN), but this is not required

    *queryData.distances = dist; // Top
    *queryData.results = data;

    KDTree::fixDown(queryData);

    queryData.maximumDistance = *queryData.distances; // Max = top of heap
}

/**
Query_rec for the unbalanced kd tree part
*/
void
KDTree::queryRec(const KDTreeNode *node, KDQuery &queryData) {
    int discriminator = node->discriminator();
    float dist;
    const KDTreeNode *nearNode;
    const KDTreeNode *farNode;

    dist = sqrDistance3D(((float *)(node->m_data)), queryData.point);

    if ( dist < queryData.maximumDistance ) {
        if ( queryData.notFilled ) {
            // Add this point anyway, because we haven't got enough positions yet.
            // We have to check for the radius only here, since if N positions
            // are added, maximumDistance <= radius
            mhInsert(queryData, ((float *)(node->m_data)), dist);
        } else {
            // Add point if distance < maximumDistance
            mhReplaceMax(queryData, ((float *)(node->m_data)), dist);
        }
    }

    // Reuse distance
    dist = ((float *)(node->m_data))[discriminator] - queryData.point[discriminator];

    if ( dist >= 0.0 ) {
        nearNode = node->loson;
        farNode = node->hison;
    } else {
        nearNode = node->hison;
        farNode = node->loson;
    }

    // Always call near node recursively
    if ( nearNode ) {
        queryRec(nearNode, queryData);
    }

    dist *= dist; // Square distance to the separator plane
    if ( farNode && (((queryData.foundN < queryData.wantedN) &&
        (dist < queryData.sqrRadius)) ||
        (dist < queryData.maximumDistance)) ) {
        // Discriminator line closer than maximumDistance : nearer positions can lie
        // on the far side. Or there are still not enough nodes found
        queryRec(farNode, queryData);
    }
}

/**
Query_rec for the unbalanced kd tree part
*/
void
KDTree::balancedQueryRec(int index, KDQuery &queryData) {
    const BalancedKDTreeNode &node = balancedRootNode[index];
    int discr = node.discriminator();
    float dist;
    int nearIndex;
    int farIndex;

    // Recursive call to the child nodes

    // Test discr (reuse distance)
    if ( index < firstLeaf ) {
        dist = ((float *)(node.m_data))[discr] - queryData.point[discr];

        if ( dist >= 0.0 ) {
            nearIndex = (index << 1) + 1; // node loson
            farIndex = nearIndex + 1; // node hison
        } else {
            farIndex = (index << 1) + 1; // node loson
            nearIndex = farIndex + 1; // node hison
        }

        // Always call near node recursively
        if ( nearIndex < numBalanced ) {
            balancedQueryRec(nearIndex, queryData);
        }

        dist *= dist; // Square distance to the separator plane
        if ((farIndex < numBalanced) && (((queryData.notFilled) && // qdat_s.foundN < qdat_s.wantedN
                                            (dist < queryData.sqrRadius)) ||
                                         (dist < queryData.maximumDistance)) ) {
            // Discriminator line closer than maximumDistance : nearer positions can lie
            // on the far side. Or there are still not enough nodes found
            balancedQueryRec(farIndex, queryData);
        }
    }

    dist = sqrDistance3D(((float *)(node.m_data)), queryData.point);

    if ( dist < queryData.maximumDistance ) {
        if ( queryData.notFilled ) {
            // Add this point anyway, because we haven't got enough positions yet.
            // We have to check for the radius only here, since if N positions
            // are added, maximumDistance <= radius
            mhInsert(queryData, ((float *)(node.m_data)), dist);
        } else {
            // Add point if distance < maximumDistance
            mhReplaceMax(queryData, ((float *)(node.m_data)), dist);
        }
    }
}

void
KDTreeNode::findMinMaxDepth(int depth, int *minDepth, int *maxDepth) const {
    if ( (loson == NULL) && (hison == NULL) ) {
        *maxDepth = Math::max(*maxDepth, depth);
        *minDepth = Math::min(*minDepth, depth);
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
inline void
KDTree::bkdswap(BalancedKDTreeNode root[], int a, int b) {
    BalancedKDTreeNode tmp = root[a];
    root[a] = root[b];
    root[b] = tmp;
}

inline float
KDTree::bkdval(BalancedKDTreeNode root[], int index, int discr) {
    return ((float *)(root[index].m_data))[discr];
}

inline void
KDTree::eSwap(BalancedKDTreeNode broot[], int a, int b) {
    KDTree::bkdswap(broot, a, b);
}

inline float
KDTree::eVal(BalancedKDTreeNode broot[], int index, int discr) {
    return KDTree::bkdval(broot, index, discr);
}

int
KDTree::getBalancedMedian(int low, int high) {
    int N = high - low + 1;  // High inclusive

    if ( N <= 1 ) {
        return low;
    }

    int FL;
    // Add 0.1 because integer powers of 2 sometimes gave a smaller FL (8 -> 2)
    FL = ((int)(Math::log(N + 0.1) / M_LN2));

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
int
KDTree::quickSelect(BalancedKDTreeNode broot[], int low, int high, int discr) {
    int median;
    int middle;
    int ll;
    int hh;

    median = KDTree::getBalancedMedian(low, high);

    for ( ;; ) {
        if ( high <= low ) {
            // One element only
            return median;
        }

        if ( high == low + 1 ) {
            // Two elements only
            if ( eVal(broot, low, discr) > eVal(broot, high, discr) ) {
                eSwap(broot, low, high);
            }
            return median;
        }

        // Find median of low, middle and high volumeListsOfItems; swap into position low
        middle = (low + high + 1) / 2;
        if ( eVal(broot, middle, discr) > eVal(broot, high, discr) ) {
            eSwap(broot, middle, high);
        }
        if ( eVal(broot, low, discr) > eVal(broot, high, discr) ) {
            eSwap(broot, low, high);
        }
        if ( eVal(broot, middle, discr) > eVal(broot, low, discr) ) {
            eSwap(broot, middle, low);
        }

        // Swap low item (now in position middle) into position (low + 1)
        eSwap(broot, middle, low + 1);

        // Nibble from each end towards middle, swapping volumeListsOfItems when stuck
        ll = low + 1;
        hh = high;
        for ( ;; ) {
            do {
                ll++;
            } while ( eVal(broot, low, discr) > eVal(broot, ll, discr) );
            do {
                hh--;
            } while ( eVal(broot, hh, discr) > eVal(broot, low, discr) );

            if ( hh < ll ) {
                break;
            }

            eSwap(broot, ll, hh);
        }

        // Swap middle item (in position low) back into correct position
        eSwap(broot, low, hh);

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
void
KDTree::copyUnbalancedRec(KDTreeNode *node, BalancedKDTreeNode *broot, int *pindex) {
    if ( node ) {
        broot[(*pindex)++].copy(*node);
        KDTree::copyUnbalancedRec(node->loson, broot, pindex);
        KDTree::copyUnbalancedRec(node->hison, broot, pindex);
    }
}

int
KDTree::bestDiscriminator(BalancedKDTreeNode broot[], int low, int high) {
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
    if ( numUnbalanced == 0 ) {
        // No balancing needed.
        return;
    }

    System::err.printf("Balancing kd-tree: %i nodes...\n", numberOfNodes);

    BalancedKDTreeNode *broot = new BalancedKDTreeNode[numberOfNodes + 1];

    broot[numberOfNodes].m_data = NULL;
    broot[numberOfNodes].m_flags = 128;

    int index = 0;

    // Copy balanced
    for ( int i = 0; i < numBalanced; i++ ) {
        broot[index++] = balancedRootNode[i];
    }

    // Copy unbalanced
    copyUnbalancedRec(root, broot, &index);

    // Clear old balanced and unbalanced part (but no data delete)
    deleteNodes(root, false);
    root = NULL;
    numUnbalanced = 0;

    deleteBNodes(false);

    numBalanced = numberOfNodes;
    BalancedKDTreeNode *dest = new BalancedKDTreeNode[numberOfNodes + 1]; // Could we do with just 1 array???

    dest[numberOfNodes].m_data = NULL;
    dest[numberOfNodes].m_flags = 64;

    // Now balance the tree recursively
    balanceRec(broot, dest, 0, 0, numberOfNodes - 1);  // High inclusive!

    balancedRootNode = dest;
    delete[] broot;

    firstLeaf = (numBalanced + 1) / 2;

    System::err.printf("done\n");
}

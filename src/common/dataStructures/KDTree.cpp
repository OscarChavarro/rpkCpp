#include <cstdio>
#include <cstring>

#include "common/dataStructures/KDTree.h"
#include "common/error.h"

#define E_SWAP(a, b) bkdswap(broot, (a), (b))
#define E_VAL(index) bkdval(broot, (index), discr)

// KD Tree with one data element per node
float *KDTree::s_distances = nullptr;

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
KDTree::DeleteNodes(KDTreeNode *node, bool deleteData) {
    if ( node == nullptr ) {
        return;
    }

    DeleteNodes(node->loson, deleteData);
    DeleteNodes(node->hison, deleteData);

    if ( deleteData ) {
        free(node->m_data);
    }

    delete node;
}

void
KDTree::DeleteBNodes(bool deleteData) {
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
    DeleteNodes(m_root, m_CopyData);
    m_root = nullptr;
    DeleteBNodes(m_CopyData);
}

void
KDTree::IterateNodes(void (*cb)(void *, void *), void *data) {
    if ( m_numUnbalanced > 0 ) {
        Error(" KDTree::IterateNodes", "Cannot iterate unbalanced trees");
        return;
    }

    for ( int i = 0; i < m_numBalanced; i++ ) {
        cb(data, m_broot[i].m_data);
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
    newNode->SetDiscriminator(0); // Default

    newNode->loson = nullptr;
    newNode->hison = nullptr;

    newPoint = (float *) data;
    m_numNodes++;
    m_numUnbalanced++;

    nodePtr = &m_root;
    parent = nullptr;

    while ( *nodePtr != nullptr ) {
        parent = *nodePtr;  // The parent

        discriminator = parent->Discriminator();

        /* Test discriminator */

        if ( newPoint[discriminator] <= ((float *) (parent->m_data))[discriminator] ) {
            nodePtr = &(parent->loson);
        } else {
            nodePtr = &(parent->hison);
        }
    }

    if ((parent != nullptr) && (parent->loson == nullptr) && (parent->hison == nullptr)) {
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

        parent->SetDiscriminator(discriminator);
        // printf("Chose discriminator %i\n", discriminator);

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

class Ckdquery {
  public:
    float *point;
    int Nwanted;
    int Nfound;
    bool notfilled;
    float **results;
    float *distances;
    float maxdist;
    float sqrRadius;
    short excludeFlags;

    void Print() const {
        printf("Point X %g, Y %g, Z %g\n", point[0], point[1], point[2]);
        printf("Nwanted %i, Nfound %i\n", Nwanted, Nfound);
        printf("maxdist %g\n", maxdist);
        printf("sqrRadius %g\n", sqrRadius);
        printf("excludeFlags %x\n", (int) excludeFlags);
    }

};

static Ckdquery qdat_s;

int
KDTree::Query(const float *point, int N, void *results,
                   float *distances, float radius, short excludeFlags) {
    int numberFound;
    float *used_distances;
    // float maxdist = HUGE;

    if ( distances == nullptr ) {
        if ( N > 1000 ) {
            Error("KDTree::Query", "Too many nodes requested");
            return 0;
        }
        used_distances = s_distances;
    } else {
        used_distances = distances;
    }

    // Fill in static class data
    qdat_s.point = (float *) point;
    qdat_s.Nwanted = N;
    qdat_s.Nfound = 0;
    qdat_s.results = (float **) results;
    qdat_s.distances = used_distances;
    qdat_s.maxdist = radius; // maxdist;
    qdat_s.sqrRadius = radius; // * radius;
    qdat_s.excludeFlags = excludeFlags;
    qdat_s.notfilled = true;

    // First query balanced part
    if ( m_broot ) {
        BQuery_rec(0);
    }

    // Now query unbalanced part using the already found nodes
    // from the balanced part
    if ( m_root ) {
        Query_rec(m_root);
    }

    numberFound = qdat_s.Nfound;

    //  printf("nrp %i, maxdist %g, maxpos %i\n", numberFound, qdat_s.maxdist,
    //	 qdat_s.maxpos);

    /*
    numberFound = Query_rec(m_root, point, N, 0, (float **)results,
                used_distances, &maxdist, 0, radius,
                excludeFlags);
    */


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
    // Ripple the node (qdat_s.Nfound) upward. There are qdat_s.Nfound + 1 nodes
    // in the tree

    int son, parent;
    float tmpDist;
    float *tmpData;

    son = qdat_s.Nfound;
    parent = (son - 1) >> 1;  // Root of tree == index 0 so parent = any son - 1 / 2

    while ((son > 0) && qdat_s.distances[parent] < qdat_s.distances[son] ) {
        tmpDist = qdat_s.distances[parent];
        tmpData = qdat_s.results[parent];

        qdat_s.distances[parent] = qdat_s.distances[son];
        qdat_s.results[parent] = qdat_s.results[son];

        qdat_s.distances[son] = tmpDist;
        qdat_s.results[son] = tmpData;

        son = parent;
        parent = (son - 1) >> 1;
    }
}

inline static void
MHInsert(float *data, float dist) {
    qdat_s.distances[qdat_s.Nfound] = dist;
    qdat_s.results[qdat_s.Nfound] = data;

    fixUp();

    // If all the photons are filled, we can use the actual maximum distance
    if ( ++qdat_s.Nfound == qdat_s.Nwanted ) {
        qdat_s.maxdist = qdat_s.distances[0];
        qdat_s.notfilled = false;
    }

    //  printf("El0 %g, Max %g\n", qdat_s.distances[0], qdat_s.maxdist);
}

inline static void
fixDown() {
    // Ripple the top node, which may not be max anymore downwards
    // There are qdat_s.Nfound nodes in the tree, starting at index 0

    int son, parent, max;
    float tmpDist;
    float *tmpData;

    max = qdat_s.Nfound;

    parent = 0;
    son = 1;

    while ( son < max ) {
        if ( qdat_s.distances[son] <= qdat_s.distances[parent] ) {
            if ((++son >= max) || qdat_s.distances[son] <= qdat_s.distances[parent] ) {
                return; // Node in place, left son and right son smaller
            }
        } else {
            if ((son + 1 < max) && qdat_s.distances[son + 1] > qdat_s.distances[son] ) {
                son++; // Take maximum of the two sons
            }
        }

        // Swap because son > parent

        tmpDist = qdat_s.distances[parent];
        tmpData = qdat_s.results[parent];

        qdat_s.distances[parent] = qdat_s.distances[son];
        qdat_s.results[parent] = qdat_s.results[son];

        qdat_s.distances[son] = tmpDist;
        qdat_s.results[son] = tmpData;

        parent = son;
        son = (parent << 1) + 1;
    }
}

inline static void
MHReplaceMax(float *data, float dist) {
    // Top = maximum element. Replace it with new and ripple down
    // The heap is full (Nfound == Nwanted), but this is not required

    *qdat_s.distances = dist; // Top
    *qdat_s.results = data;

    fixDown();

    qdat_s.maxdist = *qdat_s.distances; // Max = top of heap
}

// Query_rec for the unbalanced kdtree part
void
KDTree::Query_rec(const KDTreeNode *node) {
    //if(!node)
    //  return;

    int discr = node->Discriminator();

    float dist;
    KDTreeNode *nearNode, *farNode;

    // if(!(node->m_flags & qdat_s.excludeFlags))
    {
        dist = SqrDistance3D((float *) node->m_data, qdat_s.point);

        //printf("Q dist %g , dat %g %g %g\n", dist, ((float *)node->m_data)[0],
        //   ((float *)node->m_data)[1], ((float *)node->m_data)[2]);

        //printf("Q maxdist %g ", qdat_s.maxdist);

        if ( dist < qdat_s.maxdist ) {
            if ( qdat_s.notfilled ) // Nfound < qdat_s.Nwanted)
            {
                // Add this point anyway, because we haven't got enough points yet.
                // We have to check for the radius only here, since if N points
                // are added, maxdist <= radius

                //if(dist < qdat_s.sqrRadius)
                //      {
                MHInsert((float *) node->m_data, dist);
            } else {
                // Add point if distance < maxdist

                //if(dist < qdat_s.maxdist)
                //{
                // Add point in results

                //for(int i = 0; i<N && !FLOATEQUAL(distances[i], *maxdist, EPSILON); i++);
                //Assert(i<N, "Maxdist does not exist in distances");

                MHReplaceMax((float *) node->m_data, dist);
            }
        } // if(dist < maxdist)

        // printf("nmaxdist %g\n", qdat_s.maxdist);
    }

    // Recursive call to the child nodes

    // Test discr

    // reuse dist

    dist = ((float *) node->m_data)[discr] - qdat_s.point[discr];

    if ( dist >= 0.0 )  // qdat_s.point[discr] <= ((float *)node->m_data)[discr]
    {
        nearNode = node->loson;
        farNode = node->hison;
    } else {
        nearNode = node->hison;
        farNode = node->loson;
    }


    // Always call near node recursively

    //  int newdiscr = (discr + 1) % m_dimension;

    if ( nearNode ) {
        Query_rec(nearNode);
    }

    dist *= dist; // Square distance to the separator plane
    if ((farNode) && (((qdat_s.Nfound < qdat_s.Nwanted) &&
                       (dist < qdat_s.sqrRadius)) ||
                      (dist < qdat_s.maxdist))) {
        // Discriminator line closer than maxdist : nearer points can lie
        // on the far side. Or there are still not enough nodes found

        Query_rec(farNode);
    }
}

// Query_rec for the unbalanced kdtree part
void
KDTree::BQuery_rec(int index) {
    const BalancedKDTreeNode &node = m_broot[index];
    int discr = node.Discriminator();

    float dist;

    int nearIndex, farIndex;

    // Recursive call to the child nodes

    // Test discr (reuse dist)

    if ( index < m_firstLeaf ) {
        dist = ((float *) node.m_data)[discr] - qdat_s.point[discr];

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
        if ((farIndex < m_numBalanced) && (((qdat_s.notfilled) && // qdat_s.Nfound < qdat_s.Nwanted
                                            (dist < qdat_s.sqrRadius)) ||
                                           (dist < qdat_s.maxdist))) {
            // Discriminator line closer than maxdist : nearer points can lie
            // on the far side. Or there are still not enough nodes found
            BQuery_rec(farIndex);
        }
    }

    // if(!(node->m_flags & qdat_s.excludeFlags))
    {
        dist = SqrDistance3D((float *) node.m_data, qdat_s.point);

        if ( dist < qdat_s.maxdist ) {
            if ( qdat_s.notfilled ) // Nfound < qdat_s.Nwanted)
            {
                // Add this point anyway, because we haven't got enough points yet.
                // We have to check for the radius only here, since if N points
                // are added, maxdist <= radius

                MHInsert((float *) node.m_data, dist);
            } else {
                // Add point if distance < maxdist
                MHReplaceMax((float *) node.m_data, dist);
            }
        } // if(dist < maxdist)
    }

}

void
KDTreeNode::FindMinMaxDepth(int depth, int *minDepth, int *maxDepth) const {
    if ((loson == nullptr) && (hison == nullptr)) {
        *maxDepth = MAX(*maxDepth, depth);
        *minDepth = MIN(*minDepth, depth);
        //    printf("Min %i Max %i\n", *minDepth, *maxDepth);
    } else {
        if ( loson ) {
            loson->FindMinMaxDepth(depth + 1, minDepth, maxDepth);
        }
        if ( hison ) {
            hison->FindMinMaxDepth(depth + 1, minDepth, maxDepth);
        }
    }
}

//** KD TREE additional routines and old stuff
void
KDTree::FindMinMaxDepth(int *minDepth, int *maxDepth) {
    *minDepth = m_numNodes + 1;
    *maxDepth = 0;

    if ( m_root ) {
        m_root->FindMinMaxDepth(1, minDepth, maxDepth);
    } else {
        *minDepth = 0;
        *maxDepth = 0;
    }
}

void
KDTree::BalanceAnalysis() {
    int deepest, shortest;

    printf("KD Tree Analysis\n====================\n");

    FindMinMaxDepth(&shortest, &deepest);

    printf("Numnodes %i, Shortest %i, Deepest %i\n", m_numNodes, shortest,
           deepest);
}

/**
This Quickselect routine is adapted from the algorithm described in
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
    FL = (int) (log(N + 0.1) / M_LN2); // frexp((double)N, &FL);
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
    // int low, high ;
    int median;
    int middle, ll, hh;

    // fprintf(stderr, "quick_select: low = %i, high = %i, n = %i\n", low, high,
    // high-low+1);

    // low = 0 ; high = n-1 ;
    // median = (low + high + 1) / 2;

    median = GetBalancedMedian(low, high);

    for ( ;; ) {
        if ( high <= low ) /* One element only */
        {
            // fprintf(stderr, "Median %i\n", median);
            return median;
        }

        if ( high == low + 1 ) {  /* Two elements only */
            if ( E_VAL(low) > E_VAL(high))
                E_SWAP(low, high);
            //median++; // Take largest element, lower part is full => complete tree!!
            // fprintf(stderr, "Median %i\n", median);
            return median;
        }

        /* Find median of low, middle and high items; swap into position low */
        middle = (low + high + 1) / 2;
        if ( E_VAL(middle) > E_VAL(high))
            E_SWAP(middle, high);
        if ( E_VAL(low) > E_VAL(high))
            E_SWAP(low, high);
        if ( E_VAL(middle) > E_VAL(low))
            E_SWAP(middle, low);

        /* Swap low item (now in position middle) into position (low+1) */
        E_SWAP(middle, low + 1);

        /* Nibble from each end towards middle, swapping items when stuck */
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

        /* Swap middle item (in position low) back into correct position */
        E_SWAP(low, hh);

        /* Re-set active partition */
        if ( hh <= median ) {
            low = ll;
        }
        if ( hh >= median ) {
            high = hh - 1;
        }
    }
}

//////// Balance the kdtree

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

/*
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
        //put it in dest
        dest[destIndex] = broot[low];
        dest[destIndex].SetDiscriminator(0); // don't care...
        return;
    }

    int discr = BestDiscriminator(broot, low, high);
    // find the balance median element
    int median = quick_select(broot, low, high, discr);

    //put it in dest
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

    DeleteNodes(m_root, false);
    m_root = nullptr;
    m_numUnbalanced = 0;

    DeleteBNodes(false);

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

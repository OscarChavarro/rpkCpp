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

#ifndef K_D_TREE__
#define K_D_TREE__

#include "vsdk/toolkit/common/dataStructures/KDQuery.h"
#include "vsdk/toolkit/common/dataStructures/KDTreeNode.h"
#include "vsdk/toolkit/common/dataStructures/BalancedKDTreeNode.h"

class KDTree {
  protected:
    int numberOfNodes;
    long dataSize;
    int numUnbalanced;
    KDTreeNode *root;  // Start of non balanced part of the kd tree
    int numBalanced;
    int firstLeaf; // (numBalanced+1) / 2 : index of first leaf element
    BalancedKDTreeNode *balancedRootNode; // Start of balanced part of the kd tree
    bool copyData;
    static float *distances;

  private:
    static float sqrDistance3D(const float *a, const float *b);
    static void fixUp(KDQuery &queryData);
    static void mhInsert(KDQuery &queryData, float *data, float dist);
    static void fixDown(KDQuery &queryData);
    static void mhReplaceMax(KDQuery &queryData, float *data, float dist);
    static void bkdswap(BalancedKDTreeNode root[], int a, int b);
    static float bkdval(BalancedKDTreeNode root[], int index, int discr);
    static void eSwap(BalancedKDTreeNode broot[], int a, int b);
    static float eVal(BalancedKDTreeNode broot[], int index, int discr);
    static int getBalancedMedian(int low, int high);
    static int quickSelect(BalancedKDTreeNode broot[], int low, int high, int discr);
    static void copyUnbalancedRec(KDTreeNode *node, BalancedKDTreeNode *broot, int *pindex);
    static int bestDiscriminator(BalancedKDTreeNode broot[], int low, int high);

    void *assignData(void *data) const;
    void deleteNodes(KDTreeNode *node, bool deleteData);
    void deleteBNodes(bool deleteData);
    void queryRec(const KDTreeNode *node, KDQuery &queryData); // Unbalanced part
    void balancedQueryRec(int node, KDQuery &queryData); // Balanced part

  public:
    // Not HUGE_DOUBLE_VALUE, since we need to square it.
    static constexpr float KD_MAX_RADIUS = 1e10F;

    explicit KDTree(int dataSize, bool CopyData = true);
    virtual ~KDTree();

    void addPoint(void *data, short flags);
    void iterateNodes(void (*callBack)(void *, void *), void *data);
    void balance();

    int
    query(
        float *point,
        int N,
        void *results,
        float *inDistances = nullptr,
        float radius = KDTree::KD_MAX_RADIUS,
        short excludeFlags = 0);

    void
    balanceRec(
        BalancedKDTreeNode broot[],
        BalancedKDTreeNode dest[],
        int destIndex,
        int low,
        int high);
};

#endif

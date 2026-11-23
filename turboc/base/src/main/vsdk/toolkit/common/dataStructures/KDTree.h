#ifndef K_D_TREE__
#define K_D_TREE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/common/dataStructures/KDQuery.h"
#include "vsdk/toolkit/common/dataStructures/KDTreeNode.h"
#include "vsdk/toolkit/common/dataStructures/BalancedKDTreeNode.h"

// T provides const Vector3Df &getKDPosition() const. Radius arguments and
// returned distances are squared distances. The tree may own copies of T.
template<typename T>
class KDTreeVisitor {
  public:
    virtual ~KDTreeVisitor() {}
    virtual void visit(T &value) = 0;
};

template<typename T>
class KDTree {
  private:
    int numberOfNodes;
    int numUnbalanced;
    KDTreeNode<T> *root;
    int numBalanced;
    int firstLeaf;
    BalancedKDTreeNode<T> *balancedRootNode;
    bool copyData;

    static float coordinate(const T *data, int axis) {
        const Vector3Df &position = data->getKDPosition();
        return position.component(axis);
    }
    static float sqrDistance3D(const T *data, const float *point);
    static void fixUp(KDQuery<T> &queryData);
    static void mhInsert(KDQuery<T> &queryData, T *data, float dist);
    static void fixDown(KDQuery<T> &queryData);
    static void mhReplaceMax(KDQuery<T> &queryData, T *data, float dist);
    static void bkdswap(BalancedKDTreeNode<T> root[], int a, int b);
    static float bkdval(BalancedKDTreeNode<T> root[], int index, int discr);
    static void eSwap(BalancedKDTreeNode<T> root[], int a, int b);
    static float eVal(BalancedKDTreeNode<T> root[], int index, int discr);
    static int getBalancedMedian(int low, int high);
    static int quickSelect(BalancedKDTreeNode<T> root[], int low, int high, int discr);
    static void copyUnbalancedRec(KDTreeNode<T> *node, BalancedKDTreeNode<T> *root, int *index);
    static int bestDiscriminator(BalancedKDTreeNode<T> root[], int low, int high);
    void deleteNodes(KDTreeNode<T> *node, bool deleteData);
    void deleteBNodes(bool deleteData);
    void queryRec(const KDTreeNode<T> *node, KDQuery<T> &queryData);
    void balancedQueryRec(int node, KDQuery<T> &queryData);
    void balanceRec(BalancedKDTreeNode<T> root[], BalancedKDTreeNode<T> dest[], int index, int low, int high);

    KDTree(const KDTree &);
    KDTree &operator=(const KDTree &);

  public:
    static const float KD_MAX_RADIUS;
    explicit KDTree(bool copyData = true);
    ~KDTree();
    void addPoint(T *data, short flags);
    void balance();
    void iterateNodes(KDTreeVisitor<T> &visitor);
    int query(const Vector3Df &position, int count, T **results, float *distances = NULL,
              float radius = KD_MAX_RADIUS, short excludeFlags = 0);
    int getBalancedCount() const { return numBalanced; }
    int getUnbalancedCount() const { return numUnbalanced; }
    int getFirstLeaf() const { return firstLeaf; }
    const BalancedKDTreeNode<T> &getBalancedNode(int index) const { return balancedRootNode[index]; }
};

#include "vsdk/toolkit/common/dataStructures/KDTree.txx"

#endif

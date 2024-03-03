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
	     float *distances = nullptr, float radius = HUGE)

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

#ifndef __K_D_TREE__
#define __K_D_TREE__

// Not HUGE, since we need to square it
#define KD_MAX_RADIUS 1e10

class KDTreeNode {
  public:
    KDTreeNode *loson;
    KDTreeNode *hison;

    // Flags can be used to exclude certain nodes from a query
    // 4 LOWER BITS ARE RESERVED !!
    int m_flags;
    void *m_data;
    void findMinMaxDepth(int depth, int *minDepth, int *maxDepth) const;

    inline int discriminator() const {
        return (m_flags & 0xF);
    }

    inline void setDiscriminator(int discriminator) {
        m_flags = (m_flags & 0xFFF0) | discriminator;
    }
    inline int flags() const {
        return (m_flags & (0xFFF0));
    }
};

/**
Node for a balanced kd tree, nodes are placed in arrays
and no loson, hison pointers are necessary
*/
class BalancedKDTreeNode {
  public:
    void *m_data;
    int m_flags;

    inline void copy(const KDTreeNode &kdNode) {
        m_data = kdNode.m_data;
        m_flags = kdNode.flags();
    }

    inline int discriminator() const {
	    return (m_flags & 0xF);
    }

    inline void setDiscriminator(int discr) {
	    m_flags = (m_flags & 0xFFF0) | discr;
    }
};

class KDTree {
  protected:
    int m_numNodes;
    long m_dataSize;
    int m_numUnbalanced;
    KDTreeNode *m_root;  // Start of non balanced part of the kdtree
    int m_numBalanced;
    int m_firstLeaf; // (numBalanced+1) / 2 : index of first leaf element
    BalancedKDTreeNode *m_broot; // Start of balanced part of the kdtree
    bool m_CopyData;
    static float *s_distances;

  private:
    void *assignData(void *data) const;

    void deleteNodes(KDTreeNode *node, bool deleteData);
    void deleteBNodes(bool deleteData);
    void queryRec(const KDTreeNode *node); // Unbalanced part
    void BQuery_rec(int node); // Balanced part

  public:
    explicit KDTree(int dataSize, bool CopyData = true);
    virtual ~KDTree();

    void addPoint(void *data, short flags);

    int
    query(
        const float *point,
        int N,
        void *results,
        float *distances = nullptr,
        float radius = KD_MAX_RADIUS,
        short excludeFlags = 0);

    void iterateNodes(void (*callBack)(void *, void *), void *data);
    void balance();

    void
    balanceRec(
        BalancedKDTreeNode broot[],
        BalancedKDTreeNode dest[],
        int destIndex,
        int low,
        int high);
};

#endif

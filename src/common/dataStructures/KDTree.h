/**
 KD Tree

Creation :

KDTree( int dimension, int dataSize, bool CopyData = true )

  dimension : the dimension k of the kdtree
  dataSize : size of the datablocks that are stored in the tree.
    The first k entries of the datablock must be floats. These
    are the coordinates of this point in the kd space
  CopyData : boolean indicating if data must be copied.

virtual void addPoint(void *data)

  Adds a point to the tree. (First k entries of data == point)

Destruction :

virtual KDTree void )

  Destroys kdtree and nodes. Data is freed only when
  copy data was true.
 
Interrogation :

virtual int Query(const float *point, int N, void *results, 
	     float *distances = nullptr, float radius = HUGE)

 Gives a maximum of N points that are closest to the query point
 ('point'). An optional radius defines the maximum distance
to the query points. The number of points found is returned.

float *point : the query point (float point[k])
int N : maximum number of points to return
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


#ifndef _KDTREE_H_
#define _KDTREE_H_

#include "common/mymath.h"
#include "common/linealAlgebra/Float.h"

/*
** KD node
*/

const float KDMAXRADIUS = 1e10; // Not HUGE, since we need to square it.

class KDTreeNode {
  public:
    KDTreeNode *loson;
    KDTreeNode *hison;

    // Flags can be used to exclude certain nodes from a query
    // 4 LOWER BITS ARE RESERVED !!
    int m_flags;
    void *m_data;
    void FindMinMaxDepth(int depth, int *minDepth, int *maxDepth) const;

    inline int Discriminator() const {
        return (m_flags & 0xF);
    }

    inline void SetDiscriminator(int discriminator) {
        m_flags = m_flags & 0xFFF0 | discriminator;
    }
    inline int Flags() const {
        return (m_flags & (0xFFF0));
    }
};

// Node for a balanced kdtree, this nodes are placed in arrays
// and no loson, hison pointers are necessary
class BalancedKDTreeNode {
  public:
    void *m_data;
    int m_flags;

    inline void Copy(const KDTreeNode &kdnode) {
        m_data = kdnode.m_data;
        m_flags = kdnode.Flags();
    }

    inline int Discriminator() const {
	return (m_flags & 0xF);
    }

    inline void SetDiscriminator(int discr) {
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
    void *AssignData(void *data) const;

    void DeleteNodes(KDTreeNode *node, bool deleteData);
    void DeleteBNodes(bool deleteData);
    void FindMinMaxDepth(int *minDepth, int *maxDepth); // Unbalanced part!
    void Query_rec(const KDTreeNode *node); // Unbalanced part
    void BQuery_rec(int node); // Balanced part

  public:
    explicit KDTree(int dataSize, bool CopyData = true);
    virtual ~KDTree();

    // Add a point in the kd tree, this is always to the unbalanced part
    void addPoint(void *data, short flags);

    // Query the kd tree : both balanced and unbalanced parts taken into
    // account !  (The balanced part is searched first)
    int Query(const float *point, int N, void *results,
                      float *distances = nullptr, float radius = KDMAXRADIUS,
                      short excludeFlags = 0);

    // Iterate nodes : iterate all nodes (only for balanced ?!)
    void IterateNodes(void (*cb)(void *, void *), void *data);

    // Analyses how balanced the tree is, and prints a report to stdout
    void BalanceAnalysis();

    // Balance the tree, it is possible that a part is already balanced!!
    void Balance();

    void Balance_rec(BalancedKDTreeNode broot[], BalancedKDTreeNode dest[], int destIndex,
                             int low, int high);
};

#endif

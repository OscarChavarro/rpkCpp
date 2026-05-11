#ifndef BALANCED_K_D_TREE_NODE__
#define BALANCED_K_D_TREE_NODE__

#include "vsdk/toolkit/common/dataStructures/KDTreeNode.h"

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

#endif

#ifndef BALANCED_K_D_TREE_NODE__
#define BALANCED_K_D_TREE_NODE__

#include "vsdk/toolkit/common/dataStructures/KDTreeNode.h"

template<typename T>
class BalancedKDTreeNode {
  public:
    T *m_data;
    int m_flags;

    void copy(const KDTreeNode<T> &node) { m_data = node.m_data; m_flags = node.flags(); }
    int discriminator() const { return m_flags & 0xF; }
    void setDiscriminator(int discriminator) { m_flags = (m_flags & 0xFFF0) | discriminator; }
};

#endif

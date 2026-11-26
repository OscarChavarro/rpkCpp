#ifndef BALANCED_K_D_TREE_NODE__
#define BALANCED_K_D_TREE_NODE__

#include "vsdk/toolkit/common/dataStructures/KDTreeNode.h"

template<typename T>
class BalancedKDTreeNode {
  public:
    T *data;
    int flags;

    void copy(const KDTreeNode<T> &node) { data = node.data; flags = node.getFlags(); }
    int discriminator() const { return flags & 0xF; }
    void setDiscriminator(int discriminator) { flags = (flags & 0xFFF0) | discriminator; }
};

#endif

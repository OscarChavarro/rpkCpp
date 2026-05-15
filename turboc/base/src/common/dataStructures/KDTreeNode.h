#ifndef __K_D_TREE_NODE__
#define __K_D_TREE_NODE__

#include "common/VSDK.h"

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
        return (m_flags & 0xFFF0);
    }
};

#endif

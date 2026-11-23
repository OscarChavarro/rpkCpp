#ifndef K_D_TREE_NODE__
#define K_D_TREE_NODE__

template<typename T>
class KDTreeNode {
  public:
    KDTreeNode<T> *loson;
    KDTreeNode<T> *hison;
    int m_flags;
    T *m_data;

    void findMinMaxDepth(int depth, int *minDepth, int *maxDepth) const;
    int discriminator() const { return m_flags & 0xF; }
    void setDiscriminator(int discriminator) { m_flags = (m_flags & 0xFFF0) | discriminator; }
    int flags() const { return m_flags & 0xFFF0; }
};

#endif

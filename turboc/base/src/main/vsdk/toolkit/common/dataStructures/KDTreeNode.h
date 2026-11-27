#ifndef K_D_TREE_NODE__
#define K_D_TREE_NODE__

template<typename T>
class KDTreeNode {
  public:
    KDTreeNode<T> *loson;
    KDTreeNode<T> *hison;
    int flags;
    T *data;

    void findMinMaxDepth(int depth, int *minDepth, int *maxDepth) const;
    int discriminator() const { return flags & 0xF; }
    void setDiscriminator(int discriminator) { flags = (flags & 0xFFF0) | discriminator; }
    int getFlags() const { return flags & 0xFFF0; }
};

#endif

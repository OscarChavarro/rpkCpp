#ifndef __BINARY_TREE_NODE__
#define __BINARY_TREE_NODE__

/**
@param <T>
*/
template <class T>
class BinaryTreeNode
{
private:
    T data;
    BinaryTreeNode<T>* sibling;
    BinaryTreeNode<T>* child;

public:
    explicit BinaryTreeNode(T data) : data(data), sibling(nullptr), child(nullptr) {}

    BinaryTreeNode<T>* getSibling() { return sibling; }
    void setSibling(BinaryTreeNode<T>* s) { sibling = s; }

    BinaryTreeNode<T>* getChild() { return child; }
    void setChild(BinaryTreeNode<T>* c) { child = c; }

    T getData() { return data; }
    void setData(T d) { data = d; }
};

#endif

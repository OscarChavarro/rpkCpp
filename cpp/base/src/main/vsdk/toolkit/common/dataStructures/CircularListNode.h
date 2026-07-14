#ifndef CIRCULAR_LIST_NODE__
#define CIRCULAR_LIST_NODE__

#include "vsdk/toolkit/common/dataStructures/CircularListLink.h"

template<class T>
class CircularListNode : public CircularListLink {
  public:
    T data;
    explicit CircularListNode(const T &inData);
};

template<class T>
CircularListNode<T>::CircularListNode(const T &inData): data(inData) {
}

#endif

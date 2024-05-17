#ifndef __CIRCULAR_LIST_NODE__
#define __CIRCULAR_LIST_NODE__

#include "common/dataStructures/CircularListLink.h"

template<class T>
class CircularListNode : public CircularListLink {
  public:
    T data;
    explicit CircularListNode(const T &inData);
};

template<class T>
CircularListNode<T>::CircularListNode(const T &inData) {
    data = inData;
}

#endif

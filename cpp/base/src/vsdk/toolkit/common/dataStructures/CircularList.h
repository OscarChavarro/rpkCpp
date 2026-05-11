/**
Implementation is based on Stroustrup 'The C++ Programming Language' Section 8.3
*/

#ifndef CIRCULAR_LISTS__
#define CIRCULAR_LISTS__

#include "vsdk/toolkit/common/dataStructures/CircularListBaseIterator.h"
#include "vsdk/toolkit/common/dataStructures/CircularListNode.h"
#include "vsdk/toolkit/common/dataStructures/CircularListIterator.h"

template<class T>
class CircularList : protected CircularListBase {
  public:
    ~CircularList() override;
    virtual void add(const T &data);
    virtual void append(const T &data);
    void removeAll();
    void clear() override;
    CircularListBase &baseList();
};

template<class T>
inline CircularList<T>::~CircularList() {
}

template<class T>
inline void CircularList<T>::add(const T &data) {
    CircularListBase::addLink(new CircularListNode<T>(data));
}

template<class T>
inline void CircularList<T>::append(const T &data) {
    CircularListBase::appendLink(new CircularListNode<T>(data));
}

template<class T>
inline void CircularList<T>::removeAll() {
    CircularListNode<T> *link = static_cast<CircularListNode<T> *>(CircularListBase::remove());

    while ( link != nullptr ) {
        delete link;
        link = static_cast<CircularListNode<T> *>(CircularListBase::remove());
    }
}

template<class T>
inline void CircularList<T>::clear() {
    CircularListBase::clear();
}

template<class T>
inline CircularListBase &
CircularList<T>::baseList() {
    return *this;
}

#endif

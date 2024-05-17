#ifndef __CIRCULAR_LIST_ITERATOR__
#define __CIRCULAR_LIST_ITERATOR__

template<class T> class CircularList;

template<class T>
class CircularListIterator final : private CircularListBaseIterator {
public:
    explicit CircularListIterator(CircularList<T> &list);
    ~CircularListIterator() final;
    T *nextOnSequence();
    void init(CircularList<T> &list);
};

template<class T>
CircularListIterator<T>::CircularListIterator(CircularList<T> &list) : CircularListBaseIterator(list) {
}

template<class T>
CircularListIterator<T>::~CircularListIterator() {
}

template<class T>
inline T *CircularListIterator<T>::nextOnSequence() {
    CircularListNode<T> *link = (CircularListNode<T> *) CircularListBaseIterator::next();
    return (link ? &link->data : nullptr);
}

template<class T>
inline void CircularListIterator<T>::init(CircularList<T> &list) {
    CircularListBaseIterator::init(list);
}

#endif

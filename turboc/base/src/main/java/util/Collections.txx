#ifndef __COLLECTIONS_TXX__
#define __COLLECTIONS_TXX__

#include "java/util/ArrayList.txx"
#include "java/util/Collections.h"

template <class T> void
Collections<T>::swap(ArrayList<T> &a, ArrayList<T> &b) {
    // C++98 has no move semantics: the lists are copied
    ArrayList<T> temp(a);
    a = b;
    b = temp;
}

template <class T> void
Collections<T>::reverse(ArrayList<T> &list) {
    for ( long int i = 0, j = list.size() - 1; i < j; i++, j-- ) {
        T temp = list[i];
        list[i] = list[j];
        list[j] = temp;
    }
}

template <class T> ArrayList<T>
Collections<T>::nCopies(long int n, const T &value) {
    ArrayList<T> list;
    list.reserve(n);
    for ( long int i = 0; i < n; i++ ) {
        list.add(value);
    }
    return list;
}

#endif

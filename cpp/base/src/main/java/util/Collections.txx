#ifndef __JAVA_COLLECTIONS_TXX__
#define __JAVA_COLLECTIONS_TXX__

#include "java/util/ArrayList.txx"
#include "java/util/Collections.h"
namespace java {

template <class T> void
Collections::swap(ArrayList<T> &a, ArrayList<T> &b) {
    ArrayList<T> temp(static_cast<ArrayList<T>&&>(a));
    a = static_cast<ArrayList<T>&&>(b);
    b = static_cast<ArrayList<T>&&>(temp);
}

template <class T, class C> void
Collections::sort(ArrayList<T> &list, const C &comparator) {
    list.sort(comparator);
}

template <class T> void
Collections::reverse(ArrayList<T> &list) {
    for ( long i = 0, j = list.size() - 1; i < j; i++, j-- ) {
        T temp = list[i];
        list[i] = list[j];
        list[j] = temp;
    }
}

template <class T> ArrayList<T>
Collections::nCopies(long n, const T &value) {
    ArrayList<T> list;
    list.reserve(n);
    for ( long i = 0; i < n; i++ ) {
        list.add(value);
    }
    return list;
}

}

#endif

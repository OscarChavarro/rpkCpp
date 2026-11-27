#ifndef __COLLECTIONS__
#define __COLLECTIONS__

#include "java/util/ArrayList.h"

/**
Mirrors the role of JDK's java.util.Collections: static utility operations
on a container that don't belong on the container class itself.

C++ port note: legacy compilers (gcc 2.7, Turbo C++ 3.0) have no member
templates, so the class itself is a template on the element type, i.e.
`Collections<int>::reverse(list)`. Sorting is done by `ArrayList::sort`
(as `java.util.List.sort`).
*/
template <class T>
class Collections {
  public:
    /**
    The JDK's swap(List<?>, int, int) exchanges two elements of one list;
    since this codebase has no List, this swap instead exchanges the full
    contents of two ArrayLists.
    */
    static void swap(ArrayList<T> &a, ArrayList<T> &b);

    /**
    Reverses the order of the elements of the list.
    */
    static void reverse(ArrayList<T> &list);

    /**
    @return a list with `n` copies of `value`
    */
    static ArrayList<T> nCopies(long int n, const T &value);
};


#include "java/util/Collections.txx"

#endif

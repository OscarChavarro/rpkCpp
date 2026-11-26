#ifndef __COLLECTIONS__
#define __COLLECTIONS__

#include "java/util/ArrayList.h"
namespace java {

// Mirrors the role of JDK's java.util.Collections: static utility
// operations on a container that don't belong on the container class
// itself. The JDK's swap(List<?>, int, int) exchanges two elements of one
// list; since this codebase has no List, this swap instead exchanges the
// full contents of two ArrayLists (a 3-word pointer/field rotation via the
// already-existing move constructor/assignment, no allocation, no
// per-element copy).
class Collections final {
  public:
    template <class T>
    static void swap(ArrayList<T> &a, ArrayList<T> &b);

    /**
    Emulation of `Collections.sort(List, Comparator)`: stable sort, see
    `ArrayList::sort`.
    */
    template <class T, class C>
    static void sort(ArrayList<T> &list, const C &comparator);

    /**
    Emulation of `Collections.reverse(List)`.
    */
    template <class T>
    static void reverse(ArrayList<T> &list);

    /**
    Emulation of `Collections.nCopies(int, T)`, as a list that can be
    modified.
    */
    template <class T>
    static ArrayList<T> nCopies(long n, const T &value);
};

}

#include "java/util/Collections.txx"

#endif

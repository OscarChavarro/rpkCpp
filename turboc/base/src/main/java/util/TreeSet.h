#ifndef __TREE_SET__
#define __TREE_SET__

#include <stddef.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Object.h"
#include "java/util/ArrayList.h"

/**
Emulation of `java.util.TreeSet`: the elements are
kept sorted with their `operator<`, so the iteration order (by index, from
`0` to `size() - 1`).
*/
template <class T>
class TreeSet : public Object {
  private:
    ArrayList<T> elements;

    long lowerBound(const T &element) const;

  public:
    TreeSet();
    TreeSet(const TreeSet &other);
    ~TreeSet();

    TreeSet &operator=(const TreeSet &other);

    void clear();
    long size() const;
    BOOLEAN isEmpty() const;
    BOOLEAN contains(const T &element) const;

    /**
    @return true if the element was not present before
    */
    BOOLEAN add(const T &element);

    /**
    @return true if the element was present
    */
    BOOLEAN remove(const T &element);

    /**
    @return true if both sets have the same elements (Java `Set.equals`)
    */
    BOOLEAN equals(const TreeSet &other) const;

    // Iteration in order, 0 <= index < size()
    const T &get(long index) const;
};

#include "java/util/TreeSet.txx"

#endif

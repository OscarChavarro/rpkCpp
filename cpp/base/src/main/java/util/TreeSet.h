#ifndef __JAVA_TREE_SET__
#define __JAVA_TREE_SET__

#include "java/lang/Object.h"
#include "java/util/ArrayList.h"

namespace java {

/**
Emulation of `java.util.TreeSet`: the elements are kept sorted with their
`operator<`, and are iterated in that order (by index, from `0` to
`size() - 1`, or with a range-based for).
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
    bool isEmpty() const;
    bool contains(const T &element) const;

    /**
    @return true if the element was not present before
    */
    bool add(const T &element);

    /**
    @return true if the element was present
    */
    bool remove(const T &element);

    /**
    @return true if both sets have the same elements (Java `Set.equals`)
    */
    bool equals(const TreeSet &other) const;

    // Iteration in order, 0 <= index < size()
    const T &get(long index) const;
    const T *begin() const;
    const T *end() const;
};

}

#endif

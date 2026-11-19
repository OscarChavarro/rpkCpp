#ifndef __TREE_MAP__
#define __TREE_MAP__

#include <stddef.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Object.h"
#include "java/util/ArrayList.h"
#include "java/util/TreeMapEntry.h"

/**
Emulation of `java.util.TreeMap`: the entries are kept
sorted by key, using the key `operator<`, so the iteration order (by index,
from `0` to `size() - 1`). Each entry
is allocated on its own, so the value pointers returned by `get` stay valid
while their key is not removed.
*/
template <class K, class V>
class TreeMap : public Object {
  private:
    ArrayList<TreeMapEntry<K, V> *> entries;

    long lowerBound(const K &key) const;
    long indexOf(const K &key) const;
    void copyFrom(const TreeMap &other);

  public:
    // C++ port note: defined in the class body, calling members defined out
    // of it, as gcc 2.7.2 does not instantiate the out of class ones when
    // they are only used by the members it synthesizes
    TreeMap()
    {
    }

    TreeMap(const TreeMap &other) : Object()
    {
        copyFrom(other);
    }

    ~TreeMap()
    {
        clear();
    }

    TreeMap &operator=(const TreeMap &other)
    {
        if ( this != &other ) {
            clear();
            copyFrom(other);
        }
        return *this;
    }

    void clear();
    long size() const;
    BOOLEAN isEmpty() const;

    BOOLEAN containsKey(const K &key) const;

    /**
    @return the value of the key, or NULL when the key is not present
    */
    V *get(const K &key);
    const V *get(const K &key) const;
    V getOrDefault(const K &key, const V &defaultValue) const;

    /**
    @return true if the key was not present before
    */
    BOOLEAN put(const K &key, const V &value);

    /**
    Inserts the key with the given value only when it is not present.
    @return the value stored for the key
    */
    V &putIfAbsent(const K &key, const V &value);

    /**
    @return true if the key was present
    */
    BOOLEAN remove(const K &key);

    // Iteration in key order, 0 <= index < size()
    const K &getKey(long index) const;
    V &getValue(long index);
    const V &getValue(long index) const;
};

#include "java/util/TreeMap.txx"

#endif

#ifndef __JAVA_TREE_MAP__
#define __JAVA_TREE_MAP__

#include "java/lang/Object.h"
#include "java/util/ArrayList.h"
#include "java/util/TreeMapEntry.h"

namespace java {

/**
Emulation of `java.util.TreeMap`: the entries are kept sorted by key, using
the key `operator<`, and are iterated in that order (by index, from `0` to
`size() - 1`). Each entry is allocated on its own, so the value pointers and
references returned by `get` and `putIfAbsent` stay valid while their key
is not removed.
*/
template <class K, class V>
class TreeMap : public Object {
  private:
    ArrayList<TreeMapEntry<K, V> *> entries;

    long lowerBound(const K &key) const;
    long indexOf(const K &key) const;
    void copyFrom(const TreeMap &other);

  public:
    TreeMap();
    TreeMap(const TreeMap &other);
    ~TreeMap();

    TreeMap &operator=(const TreeMap &other);

    void clear();
    long size() const;
    bool isEmpty() const;

    bool containsKey(const K &key) const;

    /**
    @return the value of the key, or nullptr when the key is not present
    */
    V *get(const K &key);
    const V *get(const K &key) const;
    V getOrDefault(const K &key, const V &defaultValue) const;

    /**
    @return true if the key was not present before
    */
    bool put(const K &key, const V &value);

    /**
    Inserts the key with the given value only when it is not present.
    @return the value stored for the key
    */
    V &putIfAbsent(const K &key, const V &value);

    /**
    @return true if the key was present
    */
    bool remove(const K &key);

    // Iteration in key order, 0 <= index < size()
    const K &getKey(long index) const;
    V &getValue(long index);
    const V &getValue(long index) const;
};

}

#endif

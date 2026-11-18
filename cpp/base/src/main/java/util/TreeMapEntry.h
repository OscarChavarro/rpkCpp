#ifndef __JAVA_TREE_MAP_ENTRY__
#define __JAVA_TREE_MAP_ENTRY__

namespace java {

/**
Emulation of `java.util.Map.Entry` for `TreeMap`.
*/
template <class K, class V>
class TreeMapEntry {
  public:
    K key;
    V value;

    TreeMapEntry(const K &inKey, const V &inValue) :
        key(inKey),
        value(inValue)
    {
    }

    const K &
    getKey() const
    {
        return key;
    }

    V &
    getValue()
    {
        return value;
    }

    const V &
    getValue() const
    {
        return value;
    }
};

}

#endif

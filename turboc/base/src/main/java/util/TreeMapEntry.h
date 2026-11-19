#ifndef __TREE_MAP_ENTRY__
#define __TREE_MAP_ENTRY__

template <class K, class V>
class TreeMapEntry {
  public:
    K key;
    V value;

    TreeMapEntry(const K &inKey, const V &inValue):
        key(inKey),
        value(inValue)
    {
    }
};

#endif

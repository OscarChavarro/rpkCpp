#ifndef HashMapEntry__
#define HashMapEntry__

namespace java {

template <class K, class V>
class HashMapEntry {
  public:
    K key;
    V value;
    HashMapEntry *next;

    HashMapEntry(const K &inKey, const V &inValue, HashMapEntry *inNext):
        key(inKey),
        value(inValue),
        next(inNext)
    {
    }
};

}

#endif

#ifndef __HashMap__
#define __HashMap__

#include <stddef.h>

#include "java/lang/Object.h"
#include "java/util/HashMapEntry.h"

template <class K, class V>
class HashMap: public Object{ private:
    typedef HashMapEntry<K, V> Entry;

    Entry **buckets;
    long bucketCount;
    long elementCount;
    float maxLoadFactor;

    void initialize(long initialBucketCount);
    void rehash(long newBucketCount);
    long bucketIndexFor(const K &key) const;
    static size_t hashFNV1a(const unsigned char *bytes, size_t length);
    static size_t hashKeyValue(const K &value);

  public:
    HashMap();
    explicit HashMap(long initialCapacity);
    ~HashMap();

    HashMap(const HashMap &);
    HashMap &operator=(const HashMap &);

    void clear();
    long size() const;
    bool isEmpty() const;

    bool containsKey(const K &key) const;
    bool tryGet(const K &key, V *valueOut) const;
    V getOrDefault(const K &key, const V &defaultValue) const;
    bool put(const K &key, const V &value);
    bool remove(const K &key);
};

#endif

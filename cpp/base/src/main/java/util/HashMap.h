#ifndef __HASH_MAP__
#define __HASH_MAP__

#include <cstddef>
#include <cstring>

#include "java/lang/Object.h"
#include "java/lang/String.h"
#include "java/util/HashMapEntry.h"
namespace java {
template <class K, class V>
class HashMap final: public Object {
  private:
    using Entry = HashMapEntry<K, V>;

    Entry **buckets;
    long bucketCount;
    long elementCount;
    float maxLoadFactor;

    void initialize(long initialBucketCount);
    void rehash(long newBucketCount);
    long bucketIndexFor(const K &key) const;
    static size_t hashFNV1a(const unsigned char *bytes, size_t length);

    template <class T>
    static size_t hashKeyValue(const T &value);

    static size_t hashKeyValue(const java::String &value);

    template <class T>
    static size_t hashKeyValue(T *const &value);

    template <class T>
    static size_t hashKeyValue(const T *const &value);

  public:
    HashMap();
    explicit HashMap(long initialCapacity);
    HashMap(const HashMap &other);
    ~HashMap() final;

    HashMap &operator=(const HashMap &other);

    void clear();
    long size() const;
    bool isEmpty() const;

    bool containsKey(const K &key) const;
    V *get(const K &key);
    const V *get(const K &key) const;
    bool tryGet(const K &key, V *valueOut) const;
    V getOrDefault(const K &key, const V &defaultValue) const;
    bool put(const K &key, const V &value);
    bool remove(const K &key);
};
}

#include "java/util/HashMap.txx"

#endif

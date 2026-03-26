#ifndef __HashMap__
#define __HashMap__

#include "java/lang/Object.h"

namespace java {
template <class K, class V>
class HashMap final: public Object {
  private:
    class Entry;

    Entry **buckets;
    long bucketCount;
    long elementCount;
    float maxLoadFactor;

    void initialize(long initialBucketCount);
    void rehash(long newBucketCount);
    long bucketIndexFor(const K &key) const;

  public:
    HashMap();
    explicit HashMap(long initialCapacity);
    ~HashMap() final;

    HashMap(const HashMap &) = delete;
    HashMap &operator=(const HashMap &) = delete;

    void clear();
    long size() const;
    bool isEmpty() const;

    bool containsKey(const K &key) const;
    bool tryGet(const K &key, V *valueOut) const;
    V getOrDefault(const K &key, const V &defaultValue) const;
    bool put(const K &key, const V &value);
    bool remove(const K &key);
};
}

#endif

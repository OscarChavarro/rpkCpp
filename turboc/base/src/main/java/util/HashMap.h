#ifndef __HASH_MAP__
#define __HASH_MAP__

#include <stddef.h>
#include <string.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Object.h"
#include "java/lang/String.h"
#include "java/util/HashMapEntry.h"

/**
FNV-1a hash of a sequence of bytes, used by `HashMap` to place its keys.
*/
inline size_t
hashMapFnv1a(const unsigned char *bytes, size_t length) {
    const size_t offset = ((size_t)(VSDK_ULL(1469598103934665603)));
    const size_t prime = ((size_t)(VSDK_ULL(1099511628211)));
    size_t hash = offset;
    for ( size_t i = 0; i < length; i++ ) {
        hash ^= ((size_t)(bytes[i]));
        hash *= prime;
    }
    return hash;
}

/**
Hash of a `HashMap` key from its bytes: numbers, pointers and plain
structures (whose equal values have equal bytes).

C++ port note: these hash functions are not members of `HashMap`, as
legacy compilers (gcc 2.7, Turbo C++ 3.0) have no member templates.
*/
template <class T>
inline size_t
hashMapKey(const T &value) {
    return hashMapFnv1a(((const unsigned char *)(&value)), sizeof(T));
}

/**
Hash of a pointer `HashMap` key, from the bytes of the pointer (as the
template above does).

C++ port note: Turbo C++ 3.0 does not deduce `T` of `const T &` from a
pointer argument ("Could not find a match for hashMapKey(X *)"); modern
compilers keep choosing the template, an exact match, with the same hash.
*/
inline size_t
hashMapKey(const void *value) {
    return hashMapFnv1a(((const unsigned char *)(&value)), sizeof(value));
}

/**
Hash of a text `HashMap` key, from its characters.
*/
inline size_t
hashMapKey(const String &value) {
    const char *text = value.c_str();
    return hashMapFnv1a(((const unsigned char *)(text)), strlen(text));
}

template <class K, class V>
class HashMap: public Object {
  private:
    typedef HashMapEntry<K, V> Entry;

    Entry **buckets;
    long bucketCount;
    long elementCount;
    float maxLoadFactor;

    void initialize(long initialBucketCount);
    void rehash(long newBucketCount);
    long bucketIndexFor(const K &key) const;
    void copyFrom(const HashMap &other);

  public:
    // C++ port note: the constructors, the assignment and the destructor
    // are defined in the class body, calling members defined out of it: gcc
    // 2.7.2 does not instantiate the out of class ones when they are only
    // used by the members it synthesizes (as ArrayList)
    HashMap():
        buckets(NULL),
        bucketCount(0),
        elementCount(0),
        maxLoadFactor(0.75F)
    {
        initialize(16);
    }

    explicit HashMap(long initialCapacity):
        buckets(NULL),
        bucketCount(0),
        elementCount(0),
        maxLoadFactor(0.75F)
    {
        initialize(initialCapacity <= 0 ? 16 : initialCapacity);
    }

    HashMap(const HashMap &other):
        buckets(NULL),
        bucketCount(0),
        elementCount(0),
        maxLoadFactor(other.maxLoadFactor)
    {
        copyFrom(other);
    }

    ~HashMap()
    {
        clear();
    }

    HashMap &operator=(const HashMap &other)
    {
        if ( this != &other ) {
            clear();
            maxLoadFactor = other.maxLoadFactor;
            copyFrom(other);
        }
        return *this;
    }

    void clear();
    long size() const;
    BOOLEAN isEmpty() const;

    BOOLEAN containsKey(const K &key) const;
    V *get(const K &key);
    const V *get(const K &key) const;
    BOOLEAN tryGet(const K &key, V *valueOut) const;
    V getOrDefault(const K &key, const V &defaultValue) const;
    BOOLEAN put(const K &key, const V &value);
    BOOLEAN remove(const K &key);
};

#include "java/util/HashMap.txx"

#endif

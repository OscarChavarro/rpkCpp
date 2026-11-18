#include "java/util/HashMap.h"
namespace java {

template <class K, class V>
inline size_t
HashMap<K, V>::hashFNV1a(const unsigned char *bytes, size_t length) {
    const size_t offset = static_cast<size_t>(1469598103934665603ULL);
    const size_t prime = static_cast<size_t>(1099511628211ULL);
    size_t hash = offset;
    for ( size_t i = 0; i < length; i++ ) {
        hash ^= static_cast<size_t>(bytes[i]);
        hash *= prime;
    }
    return hash;
}

template <class K, class V>
template <class T>
inline size_t
HashMap<K, V>::hashKeyValue(const T &value) {
    return HashMap<K, V>::hashFNV1a(reinterpret_cast<const unsigned char *>(&value), sizeof(T));
}

template <class K, class V>
template <class T>
inline size_t
HashMap<K, V>::hashKeyValue(T *const &value) {
    return reinterpret_cast<size_t>(value);
}

template <class K, class V>
template <class T>
inline size_t
HashMap<K, V>::hashKeyValue(const T *const &value) {
    return reinterpret_cast<size_t>(value);
}

template <class K, class V>
inline size_t
HashMap<K, V>::hashKeyValue(const java::String &value) {
    const char *text = value.c_str();
    return hashFNV1a(
        reinterpret_cast<const unsigned char *>(text),
        std::strlen(text));
}

template <class K, class V>
HashMap<K, V>::HashMap():
    buckets(nullptr),
    bucketCount(0),
    elementCount(0),
    maxLoadFactor(0.75F)
{
    initialize(16);
}

template <class K, class V>
HashMap<K, V>::HashMap(long initialCapacity):
    buckets(nullptr),
    bucketCount(0),
    elementCount(0),
    maxLoadFactor(0.75F)
{
    initialize(initialCapacity <= 0 ? 16 : initialCapacity);
}

template <class K, class V>
HashMap<K, V>::HashMap(const HashMap &other):
    buckets(nullptr),
    bucketCount(0),
    elementCount(0),
    maxLoadFactor(other.maxLoadFactor)
{
    initialize(other.bucketCount <= 0 ? 16 : other.bucketCount);
    for ( long i = 0; i < other.bucketCount; i++ ) {
        Entry *current = other.buckets[i];
        while ( current != nullptr ) {
            put(current->key, current->value);
            current = current->next;
        }
    }
}

template <class K, class V>
HashMap<K, V>::~HashMap() {
    clear();
}

template <class K, class V>
HashMap<K, V> &
HashMap<K, V>::operator=(const HashMap &other) {
    if ( this == &other ) {
        return *this;
    }
    clear();
    maxLoadFactor = other.maxLoadFactor;
    initialize(other.bucketCount <= 0 ? 16 : other.bucketCount);
    for ( long i = 0; i < other.bucketCount; i++ ) {
        Entry *current = other.buckets[i];
        while ( current != nullptr ) {
            put(current->key, current->value);
            current = current->next;
        }
    }
    return *this;
}

template <class K, class V>
void
HashMap<K, V>::initialize(long initialBucketCount) {
    if ( initialBucketCount <= 0 ) {
        initialBucketCount = 16;
    }

    buckets = new Entry *[initialBucketCount];
    if ( buckets == nullptr ) {
        bucketCount = 0;
        return;
    }

    bucketCount = initialBucketCount;
    for ( long i = 0; i < bucketCount; i++ ) {
        buckets[i] = nullptr;
    }
}

template <class K, class V>
void
HashMap<K, V>::clear() {
    if ( buckets != nullptr ) {
        for ( long i = 0; i < bucketCount; i++ ) {
            Entry *current = buckets[i];
            while ( current != nullptr ) {
                Entry *next = current->next;
                delete current;
                current = next;
            }
            buckets[i] = nullptr;
        }
        delete[] buckets;
        buckets = nullptr;
    }

    bucketCount = 0;
    elementCount = 0;
}

template <class K, class V>
long
HashMap<K, V>::size() const {
    return elementCount;
}

template <class K, class V>
bool
HashMap<K, V>::isEmpty() const {
    return elementCount == 0;
}

template <class K, class V>
long
HashMap<K, V>::bucketIndexFor(const K &key) const {
    if ( bucketCount <= 0 ) {
        return 0;
    }
    const size_t hashValue = hashKeyValue(key);
    return static_cast<long>(hashValue % static_cast<size_t>(bucketCount));
}

template <class K, class V>
bool
HashMap<K, V>::containsKey(const K &key) const {
    return tryGet(key, nullptr);
}

template <class K, class V>
V *
HashMap<K, V>::get(const K &key) {
    if ( buckets == nullptr || bucketCount <= 0 ) {
        return nullptr;
    }

    const long index = bucketIndexFor(key);
    Entry *current = buckets[index];
    while ( current != nullptr ) {
        if ( current->key == key ) {
            return &current->value;
        }
        current = current->next;
    }

    return nullptr;
}

template <class K, class V>
const V *
HashMap<K, V>::get(const K &key) const {
    if ( buckets == nullptr || bucketCount <= 0 ) {
        return nullptr;
    }

    const long index = bucketIndexFor(key);
    Entry *current = buckets[index];
    while ( current != nullptr ) {
        if ( current->key == key ) {
            return &current->value;
        }
        current = current->next;
    }

    return nullptr;
}

template <class K, class V>
bool
HashMap<K, V>::tryGet(const K &key, V *valueOut) const {
    if ( buckets == nullptr || bucketCount <= 0 ) {
        return false;
    }

    const long index = bucketIndexFor(key);
    Entry *current = buckets[index];
    while ( current != nullptr ) {
        if ( current->key == key ) {
            if ( valueOut != nullptr ) {
                *valueOut = current->value;
            }
            return true;
        }
        current = current->next;
    }

    return false;
}

template <class K, class V>
V
HashMap<K, V>::getOrDefault(const K &key, const V &defaultValue) const {
    V value = defaultValue;
    if ( tryGet(key, &value) ) {
        return value;
    }
    return defaultValue;
}

template <class K, class V>
void
HashMap<K, V>::rehash(long newBucketCount) {
    if ( newBucketCount <= bucketCount || buckets == nullptr ) {
        return;
    }

    Entry **newBuckets = new Entry *[newBucketCount];
    if ( newBuckets == nullptr ) {
        return;
    }
    for ( long i = 0; i < newBucketCount; i++ ) {
        newBuckets[i] = nullptr;
    }

    for ( long i = 0; i < bucketCount; i++ ) {
        Entry *current = buckets[i];
        while ( current != nullptr ) {
            Entry *next = current->next;
            const size_t hashValue = hashKeyValue(current->key);
            const long newIndex = static_cast<long>(hashValue % static_cast<size_t>(newBucketCount));
            current->next = newBuckets[newIndex];
            newBuckets[newIndex] = current;
            current = next;
        }
    }

    delete[] buckets;
    buckets = newBuckets;
    bucketCount = newBucketCount;
}

template <class K, class V>
bool
HashMap<K, V>::put(const K &key, const V &value) {
    if ( buckets == nullptr || bucketCount <= 0 ) {
        initialize(16);
        if ( buckets == nullptr ) {
            return false;
        }
    }

    const long index = bucketIndexFor(key);
    Entry *current = buckets[index];
    while ( current != nullptr ) {
        if ( current->key == key ) {
            current->value = value;
            return true;
        }
        current = current->next;
    }

    Entry *entry = new Entry(key, value, buckets[index]);
    if ( entry == nullptr ) {
        return false;
    }
    buckets[index] = entry;
    elementCount++;

    const float maxElements = static_cast<float>(bucketCount) * maxLoadFactor;
    if ( static_cast<float>(elementCount) > maxElements ) {
        rehash(bucketCount * 2);
    }
    return true;
}

template <class K, class V>
bool
HashMap<K, V>::remove(const K &key) {
    if ( buckets == nullptr || bucketCount <= 0 ) {
        return false;
    }

    const long index = bucketIndexFor(key);
    Entry *current = buckets[index];
    Entry *previous = nullptr;
    while ( current != nullptr ) {
        if ( current->key == key ) {
            if ( previous == nullptr ) {
                buckets[index] = current->next;
            } else {
                previous->next = current->next;
            }
            delete current;
            elementCount--;
            return true;
        }
        previous = current;
        current = current->next;
    }

    return false;
}

}

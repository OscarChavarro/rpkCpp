#include <stddef.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/HashMap.h"

/**
Initializes the storage (empty) with the bucket count of `other` and copies
its entries.
*/
template <class K, class V>
void
HashMap<K, V>::copyFrom(const HashMap<K, V> &other) {
    initialize(other.bucketCount <= 0 ? 16 : other.bucketCount);
    for ( long i = 0; i < other.bucketCount; i++ ) {
        Entry *current = other.buckets[i];
        while ( current != NULL ) {
            put(current->key, current->value);
            current = current->next;
        }
    }
}

template <class K, class V>
void
HashMap<K, V>::initialize(long initialBucketCount) {
    if ( initialBucketCount <= 0 ) {
        initialBucketCount = 16;
    }

    buckets = new Entry *[initialBucketCount];
    if ( buckets == NULL ) {
        bucketCount = 0;
        return;
    }

    bucketCount = initialBucketCount;
    for ( long i = 0; i < bucketCount; i++ ) {
        buckets[i] = NULL;
    }
}

template <class K, class V>
void
HashMap<K, V>::clear() {
    if ( buckets != NULL ) {
        for ( long i = 0; i < bucketCount; i++ ) {
            Entry *current = buckets[i];
            while ( current != NULL ) {
                Entry *next = current->next;
                delete current;
                current = next;
            }
            buckets[i] = NULL;
        }
        delete[] buckets;
        buckets = NULL;
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
BOOLEAN
HashMap<K, V>::isEmpty() const {
    return elementCount == 0;
}

template <class K, class V>
long
HashMap<K, V>::bucketIndexFor(const K &key) const {
    if ( bucketCount <= 0 ) {
        return 0;
    }
    const size_t hashValue = hashMapKey(key);
    return ((long)(hashValue % ((size_t)(bucketCount))));
}

template <class K, class V>
BOOLEAN
HashMap<K, V>::containsKey(const K &key) const {
    return tryGet(key, NULL);
}

template <class K, class V>
V *
HashMap<K, V>::get(const K &key) {
    if ( buckets == NULL || bucketCount <= 0 ) {
        return NULL;
    }

    const long index = bucketIndexFor(key);
    Entry *current = buckets[index];
    while ( current != NULL ) {
        if ( current->key == key ) {
            return &current->value;
        }
        current = current->next;
    }

    return NULL;
}

template <class K, class V>
const V *
HashMap<K, V>::get(const K &key) const {
    if ( buckets == NULL || bucketCount <= 0 ) {
        return NULL;
    }

    const long index = bucketIndexFor(key);
    Entry *current = buckets[index];
    while ( current != NULL ) {
        if ( current->key == key ) {
            return &current->value;
        }
        current = current->next;
    }

    return NULL;
}

template <class K, class V>
BOOLEAN
HashMap<K, V>::tryGet(const K &key, V *valueOut) const {
    if ( buckets == NULL || bucketCount <= 0 ) {
        return FALSE;
    }

    const long index = bucketIndexFor(key);
    Entry *current = buckets[index];
    while ( current != NULL ) {
        if ( current->key == key ) {
            if ( valueOut != NULL ) {
                *valueOut = current->value;
            }
            return TRUE;
        }
        current = current->next;
    }

    return FALSE;
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
    if ( newBucketCount <= bucketCount || buckets == NULL ) {
        return;
    }

    Entry **newBuckets = new Entry *[newBucketCount];
    if ( newBuckets == NULL ) {
        return;
    }
    long i;
    for ( i = 0; i < newBucketCount; i++ ) {
        newBuckets[i] = NULL;
    }

    for ( i = 0; i < bucketCount; i++ ) {
        Entry *current = buckets[i];
        while ( current != NULL ) {
            Entry *next = current->next;
            const size_t hashValue = hashMapKey(current->key);
            const long newIndex = ((long)(hashValue % ((size_t)(newBucketCount))));
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
BOOLEAN
HashMap<K, V>::put(const K &key, const V &value) {
    if ( buckets == NULL || bucketCount <= 0 ) {
        initialize(16);
        if ( buckets == NULL ) {
            return FALSE;
        }
    }

    const long index = bucketIndexFor(key);
    Entry *current = buckets[index];
    while ( current != NULL ) {
        if ( current->key == key ) {
            current->value = value;
            return TRUE;
        }
        current = current->next;
    }

    Entry *entry = new Entry(key, value, buckets[index]);
    if ( entry == NULL ) {
        return FALSE;
    }
    buckets[index] = entry;
    elementCount++;

    const float maxElements = ((float)(bucketCount)) * maxLoadFactor;
    if ( ((float)(elementCount)) > maxElements ) {
        rehash(bucketCount * 2);
    }
    return TRUE;
}

template <class K, class V>
BOOLEAN
HashMap<K, V>::remove(const K &key) {
    if ( buckets == NULL || bucketCount <= 0 ) {
        return FALSE;
    }

    const long index = bucketIndexFor(key);
    Entry *current = buckets[index];
    Entry *previous = NULL;
    while ( current != NULL ) {
        if ( current->key == key ) {
            if ( previous == NULL ) {
                buckets[index] = current->next;
            } else {
                previous->next = current->next;
            }
            delete current;
            elementCount--;
            return TRUE;
        }
        previous = current;
        current = current->next;
    }

    return FALSE;
}


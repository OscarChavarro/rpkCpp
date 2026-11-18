#ifndef __JAVA_TREE_MAP_TXX__
#define __JAVA_TREE_MAP_TXX__

#include "java/util/ArrayList.txx"
#include "java/util/TreeMap.h"
#include "java/util/Comparator.h"

namespace java {

template <class K, class V>
TreeMap<K, V>::TreeMap()
{
}

template <class K, class V>
TreeMap<K, V>::TreeMap(const TreeMap<K, V> &other) : Object()
{
    copyFrom(other);
}

template <class K, class V>
TreeMap<K, V>::~TreeMap()
{
    clear();
}

template <class K, class V>
TreeMap<K, V> &
TreeMap<K, V>::operator=(const TreeMap<K, V> &other)
{
    if ( this != &other ) {
        clear();
        copyFrom(other);
    }
    return *this;
}

template <class K, class V>
void
TreeMap<K, V>::copyFrom(const TreeMap<K, V> &other)
{
    entries.reserve(other.entries.size());
    for ( long i = 0; i < other.entries.size(); i++ ) {
        entries.add(new TreeMapEntry<K, V>(other.entries[i]->key,
            other.entries[i]->value));
    }
}

/**
@return the index of the first entry whose key is not less than `key`
*/
template <class K, class V>
long
TreeMap<K, V>::lowerBound(const K &key) const
{
    long low = 0;
    long high = entries.size();
    while ( low < high ) {
        long middle = low + (high - low) / 2;
        if ( naturalOrderLess(entries[middle]->key, key) ) {
            low = middle + 1;
        }
        else {
            high = middle;
        }
    }
    return low;
}

/**
@return the index of the entry with the key, or -1
*/
template <class K, class V>
long
TreeMap<K, V>::indexOf(const K &key) const
{
    long index = lowerBound(key);
    if ( index < entries.size() && !naturalOrderLess(key, entries[index]->key) ) {
        return index;
    }
    return -1;
}

template <class K, class V>
void
TreeMap<K, V>::clear()
{
    for ( long i = 0; i < entries.size(); i++ ) {
        delete entries[i];
    }
    entries.clear();
}

template <class K, class V>
long
TreeMap<K, V>::size() const
{
    return entries.size();
}

template <class K, class V>
bool
TreeMap<K, V>::isEmpty() const
{
    return entries.size() == 0;
}

template <class K, class V>
bool
TreeMap<K, V>::containsKey(const K &key) const
{
    return indexOf(key) >= 0;
}

template <class K, class V>
V *
TreeMap<K, V>::get(const K &key)
{
    long index = indexOf(key);
    if ( index < 0 ) {
        return nullptr;
    }
    return &entries[index]->value;
}

template <class K, class V>
const V *
TreeMap<K, V>::get(const K &key) const
{
    long index = indexOf(key);
    if ( index < 0 ) {
        return nullptr;
    }
    return &entries[index]->value;
}

template <class K, class V>
V
TreeMap<K, V>::getOrDefault(const K &key, const V &defaultValue) const
{
    long index = indexOf(key);
    if ( index < 0 ) {
        return defaultValue;
    }
    return entries[index]->value;
}

template <class K, class V>
bool
TreeMap<K, V>::put(const K &key, const V &value)
{
    long index = lowerBound(key);
    if ( index < entries.size() && !naturalOrderLess(key, entries[index]->key) ) {
        entries[index]->value = value;
        return false;
    }
    TreeMapEntry<K, V> *entry = new TreeMapEntry<K, V>(key, value);
    if ( index == entries.size() ) {
        entries.add(entry);
    }
    else {
        entries.add(index, entry);
    }
    return true;
}

template <class K, class V>
V &
TreeMap<K, V>::putIfAbsent(const K &key, const V &value)
{
    long index = lowerBound(key);
    if ( index < entries.size() && !naturalOrderLess(key, entries[index]->key) ) {
        return entries[index]->value;
    }
    TreeMapEntry<K, V> *entry = new TreeMapEntry<K, V>(key, value);
    if ( index == entries.size() ) {
        entries.add(entry);
    }
    else {
        entries.add(index, entry);
    }
    return entry->value;
}

template <class K, class V>
bool
TreeMap<K, V>::remove(const K &key)
{
    long index = indexOf(key);
    if ( index < 0 ) {
        return false;
    }
    delete entries[index];
    entries.remove(index);
    return true;
}

template <class K, class V>
const K &
TreeMap<K, V>::getKey(long index) const
{
    return entries[index]->key;
}

template <class K, class V>
V &
TreeMap<K, V>::getValue(long index)
{
    return entries[index]->value;
}

template <class K, class V>
const V &
TreeMap<K, V>::getValue(long index) const
{
    return entries[index]->value;
}

}

#endif

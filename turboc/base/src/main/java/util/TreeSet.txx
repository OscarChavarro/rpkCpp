#ifndef __TREE_SET_TXX__
#define __TREE_SET_TXX__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.txx"
#include "java/util/TreeSet.h"
#include "java/util/Comparator.h"

template <class T>
TreeSet<T>::TreeSet()
{
}

template <class T>
TreeSet<T>::TreeSet(const TreeSet<T> &other) : Object(), elements(other.elements)
{
}

template <class T>
TreeSet<T>::~TreeSet()
{
}

template <class T>
TreeSet<T> &
TreeSet<T>::operator=(const TreeSet<T> &other)
{
    elements = other.elements;
    return *this;
}

/**
@return the index of the first element that is not less than `element`
*/
template <class T>
long
TreeSet<T>::lowerBound(const T &element) const
{
    long low = 0;
    long high = elements.size();
    while ( low < high ) {
        long middle = low + (high - low) / 2;
        if ( naturalOrderLess(elements[middle], element) ) {
            low = middle + 1;
        }
        else {
            high = middle;
        }
    }
    return low;
}

template <class T>
void
TreeSet<T>::clear()
{
    elements.clear();
}

template <class T>
long
TreeSet<T>::size() const
{
    return elements.size();
}

template <class T>
BOOLEAN
TreeSet<T>::isEmpty() const
{
    return elements.size() == 0;
}

template <class T>
BOOLEAN
TreeSet<T>::contains(const T &element) const
{
    long index = lowerBound(element);
    return index < elements.size() && !naturalOrderLess(element, elements[index]);
}

template <class T>
BOOLEAN
TreeSet<T>::add(const T &element)
{
    long index = lowerBound(element);
    if ( index < elements.size() && !naturalOrderLess(element, elements[index]) ) {
        return FALSE;
    }
    if ( index == elements.size() ) {
        elements.add(element);
    }
    else {
        elements.add(index, element);
    }
    return TRUE;
}

template <class T>
BOOLEAN
TreeSet<T>::remove(const T &element)
{
    long index = lowerBound(element);
    if ( index < elements.size() && !naturalOrderLess(element, elements[index]) ) {
        elements.remove(index);
        return TRUE;
    }
    return FALSE;
}

template <class T>
BOOLEAN
TreeSet<T>::equals(const TreeSet<T> &other) const
{
    if ( elements.size() != other.elements.size() ) {
        return FALSE;
    }
    for ( long i = 0; i < elements.size(); i++ ) {
        if ( naturalOrderLess(elements[i], other.elements[i]) ||
             naturalOrderLess(other.elements[i], elements[i]) ) {
            return FALSE;
        }
    }
    return TRUE;
}

template <class T>
const T &
TreeSet<T>::get(long index) const
{
    return elements[index];
}

#endif

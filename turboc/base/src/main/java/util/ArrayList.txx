#ifndef __ARRAY_LIST_TXX__
#define __ARRAY_LIST_TXX__

#include <stddef.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.h"

/**
@return a new array of `n` elements, or NULL when `n` is not positive. The
first argument only selects `T` (legacy compilers lack explicit template
arguments for functions).

C++ port note: gcc 2.7.2 with -fhandle-exceptions crashes when a member
function of a class template stores the result of `new T[]` and then
tests it, so `ArrayList::init` allocates through this function.
*/
template <class T>
T *
newArrayListStorage(T *, long int n)
{
    if (n > 0) {
        return new T[n];
    }
    return NULL;
}

template <class T>
void
ArrayList<T>::init()
{
    Data = newArrayListStorage((T *)NULL, maxSize);
    if (maxSize > 0 && !Data) {
        maxSize = 0;
    }
    currentSize = 0;
}

template <class T>
void
ArrayList<T>::dispose() {
    if ( Data ) {
        delete[] Data;
        Data = NULL;
    }
    currentSize = 0;
    maxSize = 0;
}

template <class T>
void
ArrayList<T>::copyFrom(const ArrayList<T>& other)
{
    increaseChunk = other.increaseChunk;
    currentSize = other.currentSize;
    maxSize = other.maxSize;
    Data = NULL;
    if (maxSize > 0) {
        Data = new T[maxSize];
        for (long int i = 0; i < currentSize; i++) {
            Data[i] = other.Data[i];
        }
    }
}

template <class T>
void
ArrayList<T>::assignFrom(const ArrayList<T>& other)
{
    if (this == &other) return;
    delete[] Data;
    copyFrom(other);
}

template <class T> BOOLEAN
ArrayList<T>::add(T voxelData)
{
    if ( currentSize >= maxSize ) {
        // Grows by half of the capacity, as java.util.ArrayList, and at
        // least by the increase chunk: appending is amortized constant time
        long int growth = maxSize / 2;
        if ( growth < increaseChunk ) {
            growth = increaseChunk;
        }
        long int newMaxSize = maxSize + growth;
        if ( newMaxSize <= maxSize ) {
            newMaxSize = maxSize + 1;
        }

        T *newData = new T[newMaxSize];
        if ( !newData ) {
            return FALSE;
        }

        for ( long int i = 0; i < currentSize; i++ ) {
            newData[i] = Data[i];
        }

        delete[] Data;
        Data = newData;
        maxSize = newMaxSize;
    }
    Data[currentSize] = voxelData;
    currentSize++;
    return TRUE;
}

template <class T> long int
ArrayList<T>::size() const {
    return currentSize;
}

template <class T> BOOLEAN
ArrayList<T>::isEmpty() const {
    return currentSize == 0;
}

template <class T> T &
ArrayList<T>::operator[](long int i) {
    return Data[i];
}

template <class T> const T &
ArrayList<T>::operator[](long int i) const {
    return Data[i];
}

template <class T> T
ArrayList<T>::get(long int i) const {
    return Data[i];
}

template <class T> T*
ArrayList<T>::data() {
    return Data;
}

template <class T> const T*
ArrayList<T>::data() const {
    return Data;
}

template <class T> void
ArrayList<T>::reserve(long int n) {
    if (n <= maxSize) return;
    T* newData = new T[n];
    for (long int i = 0; i < currentSize; i++) {
        newData[i] = Data[i];
    }
    delete[] Data;
    Data = newData;
    maxSize = n;
}

template <class T> void
ArrayList<T>::add(long int pos, T elem)
{
    if ( size() == 0 ) {
        add(elem);
        return;
    }
    T last = get(size() - 1);
    add(last);
    int i;
    for ( i = size() - 1; i >= 1; i-- ) {
        if ( i == pos ) {
            Data[i] = elem;
            break;
        }
        Data[i] = Data[i - 1];
    }
    if ( i <= 0 ) {
        Data[0] = elem;
    }
}

template <class T> void
ArrayList<T>::remove(long int pos)
{
    for ( long i = pos; i < size()-1; i++ ) {
        Data[i] = Data[i+1];
    }
    currentSize--;
}

template <class T> void
#ifdef __TURBOC__
ArrayList<T>::remove2(T data)
#else
ArrayList<T>::remove(T data)
#endif
{
    BOOLEAN shouldRemove = FALSE;
    long int i;
    for ( i = 0; i < size(); i++ ) {
        if ( vsdkEquals(Data[i], data) ) {
            shouldRemove = TRUE;
            break;
        }
    }

    if ( shouldRemove ) {
        remove(i);
    }
}

template <class T> void
ArrayList<T>::set(long int pos, T elem) {
    Data[pos] = elem;
}

template <class T> void
ArrayList<T>::clear() {
    currentSize = 0;
}

template <class T> long int
ArrayList<T>::indexOf(const T &elem) const {
    for ( long int i = 0; i < currentSize; i++ ) {
        if ( vsdkEquals(Data[i], elem) ) {
            return i;
        }
    }
    return -1;
}

template <class T> BOOLEAN
ArrayList<T>::contains(const T &elem) const {
    return indexOf(elem) >= 0;
}

/**
Sorts the elements in [from, to), using `buffer` as temporary storage.
*/
template <class T> void
ArrayList<T>::mergeSort(T *buffer, long int from, long int to,
    const Comparator<T> &comparator)
{
    if ( to - from < 2 ) {
        return;
    }
    long int middle = from + (to - from) / 2;
    mergeSort(buffer, from, middle, comparator);
    mergeSort(buffer, middle, to, comparator);
    if ( comparator.compare(Data[middle - 1], Data[middle]) <= 0 ) {
        return;
    }
    long int i;
    for ( i = from; i < middle; i++ ) {
        buffer[i] = Data[i];
    }
    long int left = from;
    long int right = middle;
    long int target = from;
    while ( left < middle && right < to ) {
        // Takes the right element only when strictly smaller: stable
        if ( comparator.compare(Data[right], buffer[left]) < 0 ) {
            Data[target++] = Data[right++];
        }
        else {
            Data[target++] = buffer[left++];
        }
    }
    while ( left < middle ) {
        Data[target++] = buffer[left++];
    }
}

template <class T> void
ArrayList<T>::sort(const Comparator<T> &comparator) {
    if ( currentSize < 2 ) {
        return;
    }
    T *buffer = new T[currentSize];
    mergeSort(buffer, 0, currentSize, comparator);
    delete[] buffer;
}

template <class T> BOOLEAN
ArrayList<T>::equals(const ArrayList<T> &other) const {
    if ( currentSize != other.currentSize ) {
        return FALSE;
    }
    for ( long int i = 0; i < currentSize; i++ ) {
        if ( !vsdkEquals(Data[i], other.Data[i]) ) {
            return FALSE;
        }
    }
    return TRUE;
}

template <class T> void
ArrayList<T>::addAll(const ArrayList<T> &other) {
    if ( this == &other ) {
        ArrayList<T> copy(other);
        addAll(copy);
        return;
    }
    reserve(currentSize + other.currentSize);
    for ( long int i = 0; i < other.currentSize; i++ ) {
        add(other.Data[i]);
    }
}


#endif

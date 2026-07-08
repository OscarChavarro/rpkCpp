#include "java/util/ArrayList.h"


template <class T>
ArrayList<T>::ArrayList() {
    currentSize = 0;
    increaseChunk = 100;
    maxSize = increaseChunk;
    init();
}

template <class T>
ArrayList<T>::ArrayList(long i) {
    currentSize = 0;
    increaseChunk = i;
    maxSize = increaseChunk;
    init();
}

template <class T>
ArrayList<T>::~ArrayList() {
    dispose();
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

template <class T> void
ArrayList<T>::init() {
    Data = NULL;
    currentSize = 0;
}

template <class T> bool
ArrayList<T>::add(T voxelData)
{
    if ( Data == NULL ) {
        if ( maxSize <= 0 ) {
            maxSize = 1;
        }
        Data = new T[maxSize];
        if ( !Data ) {
            maxSize = 0;
            return false;
        }
    } else if ( currentSize >= maxSize ) {
        long int newMaxSize = maxSize * 2;
        if ( newMaxSize <= maxSize ) {
            newMaxSize = maxSize + 1;
        }

        T *newData = new T[newMaxSize];
        if ( !newData ) {
            return false;
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
    return true;
}

template <class T> long int
ArrayList<T>::size() const {
    return currentSize;
}

template <class T> T &
ArrayList<T>::operator[](long int i) {
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

template <class T> void
ArrayList<T>::add(long int pos, T elem)
{
    if ( currentSize == 0 ) {
        add(elem);
        return;
    }

    int lastPosition = size() - 1;
    T last = get(lastPosition);
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
ArrayList<T>::remove(T data)
{
    bool shouldRemove = false;
    long int i;
    for ( i = 0; i < size(); i++ ) {
        if ( Data[i] == data ) {
            shouldRemove = true;
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

template <class T> void
ArrayList<T>::truncate(long int size) {
    if ( size < 0 ) {
        currentSize = 0;
    } else if ( size < currentSize ) {
        currentSize = size;
    }
}

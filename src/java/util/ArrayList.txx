#include <cstdio>
#include <cstdlib>

#include "java/util/ArrayList.h"

namespace java {

template <class T> bool
ArrayList<T>::add(T voxelData)
{
    if ( currentSize >= maxSize ) {
        Data = (T *)realloc(Data, sizeof(T) * (maxSize + increaseChunk));
        if ( !Data ) {
            return false;
        }
        maxSize += increaseChunk;
    }
    Data[currentSize] = voxelData;
    currentSize++;
    return true;
}

template <class T> void
ArrayList<T>::add(long int pos, T voxelData)
{
    T last = get(size()-1);
    add(last);
    int i;
    for ( i = size()-2; i > 0; i-- ) {
        if ( i == pos ) {
            Data[i] = voxelData;
            break;
        }
        Data[i] = Data[i-1];
    }
}

template <class T> void
ArrayList<T>::remove(long int pos)
{
    int i;
    for ( i = pos; i < size()-1; i++ ) {
        Data[i] = Data[i+1];
    }
    currentSize--;
}

template <class T> void
ArrayList<T>::elim(void)
{
    if ( Data ) {
        free(Data);
        Data = NULL;
        currentSize = 0;
    }
}

}

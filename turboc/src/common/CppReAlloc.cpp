#include <string.h>

#include "common/CppReAlloc.h"

unsigned char *
CppReAlloc::reAlloc(
    unsigned char *ptr,
    int oldElementCount,
    int newElementCount)
{
    if ( newElementCount <= 0 ) {
        delete[] ptr;
        return NULL;
    }

    unsigned char *newPtr = new unsigned char[newElementCount];
    if ( newPtr == NULL ) {
        return NULL;
    }

    if ( ptr != NULL && oldElementCount > 0 ) {
        const int copyElementCount = oldElementCount < newElementCount ? oldElementCount : newElementCount;
        memcpy(
            newPtr,
            ptr,
            ((unsigned long)(copyElementCount)) * sizeof(unsigned char));
        delete[] ptr;
    }

    return newPtr;
}

char **
CppReAlloc::reAlloc(
    char **ptr,
    int oldElementCount,
    int newElementCount)
{
    if ( newElementCount <= 0 ) {
        delete[] ptr;
        return NULL;
    }

    char **newPtr = new char *[newElementCount];
    if ( newPtr == NULL ) {
        return NULL;
    }

    if ( ptr != NULL && oldElementCount > 0 ) {
        const int copyElementCount = oldElementCount < newElementCount ? oldElementCount : newElementCount;
        memcpy(
            newPtr,
            ptr,
            ((unsigned long)(copyElementCount)) * sizeof(char *));
        delete[] ptr;
    }

    return newPtr;
}

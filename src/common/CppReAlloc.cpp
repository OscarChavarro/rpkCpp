#include <cstring>

#include "common/CppReAlloc.h"

unsigned char *
CppReAlloc::reAlloc(
    unsigned char *ptr,
    int oldElementCount,
    int newElementCount)
{
    if ( newElementCount <= 0 ) {
        delete[] ptr;
        return nullptr;
    }

    unsigned char *newPtr = new unsigned char[newElementCount];
    if ( newPtr == nullptr ) {
        return nullptr;
    }

    if ( ptr != nullptr && oldElementCount > 0 ) {
        const int copyElementCount = oldElementCount < newElementCount ? oldElementCount : newElementCount;
        std::memcpy(
            newPtr,
            ptr,
            static_cast<unsigned long>(copyElementCount) * sizeof(unsigned char));
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
        return nullptr;
    }

    char **newPtr = new char *[newElementCount];
    if ( newPtr == nullptr ) {
        return nullptr;
    }

    if ( ptr != nullptr && oldElementCount > 0 ) {
        const int copyElementCount = oldElementCount < newElementCount ? oldElementCount : newElementCount;
        std::memcpy(
            newPtr,
            ptr,
            static_cast<unsigned long>(copyElementCount) * sizeof(char *));
        delete[] ptr;
    }

    return newPtr;
}

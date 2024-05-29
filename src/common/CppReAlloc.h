#ifndef __CPP_RE_ALLOC__
#define __CPP_RE_ALLOC__

#include <cstring>

template<typename T>
class CppReAlloc {
  public:
    T* reAlloc(T* ptr, size_t newElementCount) {
        if (newElementCount == 0) {
            delete[] ptr;
            return nullptr;
        }

        if (ptr == nullptr) {
            return new T[newElementCount];
        }

        T* newPtr = new T[newElementCount];
        if (newPtr == nullptr) {
            return nullptr;
        }

        size_t oldElementCount = newElementCount; // Placeholder: should be the actual old size
        size_t copyElementCount = oldElementCount < newElementCount ? oldElementCount : newElementCount;
        std::memcpy(newPtr, ptr, copyElementCount * sizeof(T));
        delete[] ptr;
        return newPtr;
    }
};

#endif

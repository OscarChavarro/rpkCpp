#ifndef __ARRAY_LIST__
#define __ARRAY_LIST__

#include <stddef.h>
#include <string.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Object.h"
#include "java/util/Comparator.h"

/**
Equality of the elements of ArrayList (indexOf, contains, remove(T) and
equals). Turbo C++ 3.0 compiles every member of the class templates it uses,
also for element types without operator==, so there the generic version
compares the bytes (as == does for numbers and pointers; its arguments go
by value, as it does not deduce `const T &` from pointers), and the classes
with operator== declare an exact overload (taken before the template) that
calls it. VSDK_CHECK_ELEMENT_EQUALITY leaves the generic version undefined,
so linking with a modern compiler names the element types compared with it.
*/
#if defined(__TURBOC__)
template <class T> BOOLEAN
vsdkEquals(T a, T b)
{
    return memcmp(&a, &b, sizeof(T)) == 0;
}
#elif defined(VSDK_CHECK_ELEMENT_EQUALITY)
template <class T> BOOLEAN
vsdkEquals(const T &a, const T &b);
#else
template <class T> BOOLEAN
vsdkEquals(const T &a, const T &b)
{
    return a == b;
}
#endif

inline BOOLEAN
vsdkEquals(const double &a, const double &b)
{
    return a == b;
}

inline BOOLEAN
vsdkEquals(const float &a, const float &b)
{
    return a == b;
}

template<class T>
class ArrayList : public Object {
private:
    long int increaseChunk;
    long int currentSize;
    long int maxSize;
    T *Data;

    void mergeSort(T *buffer, long int from, long int to,
                   const Comparator<T> &comparator);

    void init();
    void copyFrom(const ArrayList& other);
    void assignFrom(const ArrayList& other);

public:
    // C++ port note: the constructors, the assignment and the destructor are
    // defined in the class body, calling members defined out of it: gcc
    // 2.7.2 does not instantiate the out of class ones when they are only
    // used by the copy constructors and assignments it synthesizes
    ArrayList()
    {
        // Storage is allocated on the first insertion, as many lists of the
        // port stay empty (saves memory on 16 bit targets)
        currentSize = 0;
        increaseChunk = 100;
        maxSize = 0;
        init();
    }
    explicit ArrayList(long i)
    {
        currentSize = 0;
        increaseChunk = i;
        maxSize = increaseChunk;
        init();
    }
    ArrayList(const ArrayList& other) : Data(NULL)
    {
        copyFrom(other);
    }

    ArrayList& operator=(const ArrayList& other)
    {
        assignFrom(other);
        return *this;
    }

    ~ArrayList()
    {
        dispose();
    }

    void dispose();

    long int size() const;
    BOOLEAN isEmpty() const;
    T get(long int i) const;

    BOOLEAN add(T elem);
    T &operator[](long int i);
    const T &operator[](long int i) const;
    T *data();
    const T *data() const;
    void reserve(long int n);
    void add(long int pos, T elem);
    void remove(long int pos);
#ifdef __TURBOC__
    // Turbo C++ 3.0 represents long long as long.  A remove(T) overload
    // would then collide with remove(long) when T is long long.
    void remove2(T data);
#else
    void remove(T data);
#endif
    void set(long int pos, T elem);
    void clear();

    /**
    @return the index of the first element equal (vsdkEquals) to `elem`,
    or -1
    */
    long int indexOf(const T &elem) const;
    BOOLEAN contains(const T &elem) const;
    void addAll(const ArrayList &other);

    /**
    @return true if both lists have equal (vsdkEquals) elements in the
    same order (Java `List.equals`)
    */
    BOOLEAN equals(const ArrayList &other) const;

    /**
    Emulation of `java.util.List.sort`: stable merge sort (equal elements
    keep their relative order).
    */
    void sort(const Comparator<T> &comparator);
};

#include "java/util/ArrayList.txx"

#endif

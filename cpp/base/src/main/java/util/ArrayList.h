#ifndef __ARRAY_LIST__
#define __ARRAY_LIST__

#include "java/lang/Object.h"

namespace java {
    template<class T>
    class ArrayList final : public Object {
    private:
        long int increaseChunk;
        long int currentSize;
        long int maxSize;
        T *Data;

        void init()
        {
            if (maxSize > 0) {
                Data = new T[maxSize];
                if (!Data) {
                    maxSize = 0;
                }
            } else {
                Data = nullptr;
            }
            currentSize = 0;
        }

    public:
        ArrayList();
        explicit ArrayList(long i)
        {
            currentSize = 0;
            increaseChunk = i;
            maxSize = increaseChunk;
            init();
        }
        ArrayList(const ArrayList& other);
        ArrayList& operator=(const ArrayList& other);
        ArrayList(ArrayList&& other) noexcept;
        ArrayList& operator=(ArrayList&& other) noexcept;
        ~ArrayList();

        void dispose();

        long int size() const;
        T get(long int i) const;

        bool add(T elem);
        T &operator[](long int i);
        const T &operator[](long int i) const;
        T *data();
        const T *data() const;
        void reserve(long int n);
        void add(long int pos, T elem);
        void remove(long int pos);
        void remove(T data);
        void set(long int pos, T elem);
        void clear();

        bool isEmpty() const;

        /**
        @return the index of the first element equal (`==`) to `elem`, or -1
        */
        long int indexOf(const T &elem) const;
        bool contains(const T &elem) const;
        void addAll(const ArrayList &other);

        /**
        @return true if both lists have equal (`==`) elements in the same
        order (Java `List.equals`)
        */
        bool equals(const ArrayList &other) const;

        // Range-based for, as the Java enhanced for statement
        T *begin();
        T *end();
        const T *begin() const;
        const T *end() const;

        /**
        Emulation of `java.util.List.sort`: stable merge sort (equal elements
        keep their relative order). `comparator` is a `Comparator<T>`, a
        lambda `(const T &a, const T &b) -> int` with the same convention,
        or a C++ ordering predicate `(const T &a, const T &b) -> bool` (true
        when `a` goes before `b`).
        */
        template <class C>
        void sort(const C &comparator);

        /**
        As `list.sort(null)` in Java: natural order (`operator<`).
        */
        void sort(decltype(nullptr));

    private:
        template <class C>
        void mergeSort(T *buffer, long int from, long int to, const C &comparator);
    };
}

#endif

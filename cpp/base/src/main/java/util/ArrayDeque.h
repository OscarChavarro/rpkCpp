#ifndef __ARRAY_DEQUE__
#define __ARRAY_DEQUE__

#include "java/lang/Object.h"
namespace java {
    template <class E>
    class ArrayDeque final : public Object {
    private:
        E *elements;
        long head;
        long tail;
        long capacity;
        static const long MIN_INITIAL_CAPACITY = 16;

        void doubleCapacity();
        long mask() const;
        void allocate(long numElements);

    public:
        ArrayDeque();
        explicit ArrayDeque(long numElements);
        ArrayDeque(const ArrayDeque& other);
        ArrayDeque& operator=(const ArrayDeque& other);
        ~ArrayDeque();

        // Capacity operations
        long size() const;
        bool isEmpty() const;
        void clear();

        // Add operations
        void addFirst(const E& e);
        void addLast(const E& e);
        void push(const E& e);

        // Remove operations
        E removeFirst();
        E removeLast();
        E pop();

        // Peek operations
        E peekFirst() const;
        E peekLast() const;

        // Poll operations
        E pollFirst();
        E pollLast();

        // Utility
        void dispose();

        // Iterator-like access (minimal)
        E get(long index) const;
    };
}

#include "java/util/ArrayDeque.txx"

#endif

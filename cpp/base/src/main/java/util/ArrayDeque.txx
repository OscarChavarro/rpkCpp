#include "java/util/ArrayDeque.h"
namespace java {

template <class E>
ArrayDeque<E>::ArrayDeque() {
    allocate(MIN_INITIAL_CAPACITY);
}

template <class E>
ArrayDeque<E>::ArrayDeque(long numElements) {
    long initialCapacity = MIN_INITIAL_CAPACITY;
    while (initialCapacity < numElements + 1) {
        initialCapacity <<= 1;
    }
    allocate(initialCapacity);
}

template <class E>
ArrayDeque<E>::ArrayDeque(const ArrayDeque& other) {
    allocate(other.capacity);
    head = other.head;
    tail = other.tail;

    if (head != tail) {
        long i = head;
        long mask = capacity - 1;
        while (i != tail) {
            elements[i & mask] = other.elements[i & mask];
            i++;
        }
    }
}

template <class E>
ArrayDeque<E>& ArrayDeque<E>::operator=(const ArrayDeque& other) {
    if (this != &other) {
        dispose();
        allocate(other.capacity);
        head = other.head;
        tail = other.tail;

        if (head != tail) {
            long i = head;
            long mask = capacity - 1;
            while (i != tail) {
                elements[i & mask] = other.elements[i & mask];
                i++;
            }
        }
    }
    return *this;
}

template <class E>
ArrayDeque<E>::~ArrayDeque() {
    dispose();
}

template <class E>
void ArrayDeque<E>::allocate(long numElements) {
    capacity = numElements;
    elements = new E[capacity];
    head = 0;
    tail = 0;
}

template <class E>
void ArrayDeque<E>::dispose() {
    if (elements) {
        delete[] elements;
        elements = nullptr;
    }
    head = 0;
    tail = 0;
    capacity = 0;
}

template <class E>
long ArrayDeque<E>::mask() const {
    return capacity - 1;
}

template <class E>
long ArrayDeque<E>::size() const {
    long m = mask();
    return (tail - head) & m;
}

template <class E>
bool ArrayDeque<E>::isEmpty() const {
    return head == tail;
}

template <class E>
void ArrayDeque<E>::clear() {
    head = 0;
    tail = 0;
}

template <class E>
void ArrayDeque<E>::doubleCapacity() {
    long m = mask();
    long p = head;
    long n = capacity;
    long r = n - p;
    long newCapacity = n << 1;

    E *a = new E[newCapacity];

    for (long i = 0; i < r; i++) {
        a[i] = elements[p + i];
    }
    for (long i = 0; i < p; i++) {
        a[r + i] = elements[i];
    }

    delete[] elements;
    elements = a;
    capacity = newCapacity;
    head = 0;
    tail = n;
}

template <class E>
void ArrayDeque<E>::addFirst(const E& e) {
    long m = mask();
    long i = (head - 1) & m;
    if (i == (tail & m)) {
        doubleCapacity();
        m = mask();
        i = (head - 1) & m;
    }
    head = i;
    elements[i] = e;
}

template <class E>
void ArrayDeque<E>::addLast(const E& e) {
    long m = mask();
    long i = tail;
    elements[i & m] = e;
    tail = (i + 1) & m;
    if (tail == (head & m)) {
        doubleCapacity();
    }
}

template <class E>
void ArrayDeque<E>::push(const E& e) {
    addFirst(e);
}

template <class E>
E ArrayDeque<E>::removeFirst() {
    long m = mask();
    long i = head;
    E result = elements[i & m];
    head = (i + 1) & m;
    return result;
}

template <class E>
E ArrayDeque<E>::removeLast() {
    long m = mask();
    long i = (tail - 1) & m;
    E result = elements[i & m];
    tail = i;
    return result;
}

template <class E>
E ArrayDeque<E>::pop() {
    return removeFirst();
}

template <class E>
E ArrayDeque<E>::peekFirst() const {
    long m = mask();
    return elements[head & m];
}

template <class E>
E ArrayDeque<E>::peekLast() const {
    long m = mask();
    return elements[(tail - 1) & m];
}

template <class E>
E ArrayDeque<E>::pollFirst() {
    if (isEmpty()) {
        return E();
    }
    return removeFirst();
}

template <class E>
E ArrayDeque<E>::pollLast() {
    if (isEmpty()) {
        return E();
    }
    return removeLast();
}

template <class E>
E ArrayDeque<E>::get(long index) const {
    if (index < 0 || index >= size()) {
        return E();
    }
    long m = mask();
    return elements[(head + index) & m];
}

}

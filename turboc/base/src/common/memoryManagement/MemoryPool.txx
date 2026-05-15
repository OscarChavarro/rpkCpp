#include "common/memoryManagement/MemoryPool.h"

template <class T>
MemoryPool<T>::MemoryPool():
    head(NULL),
    tail(NULL),
    current(NULL),
    initialized(false)
{
}

template <class T>
MemoryPool<T>::~MemoryPool() {
    Block *it = head;
    while ( it != NULL ) {
        Block *next = it->next;
        destroyBlock(it);
        it = next;
    }
    head = NULL;
    tail = NULL;
    current = NULL;
    initialized = false;
}

template <class T>
typename MemoryPool<T>::Block *
MemoryPool<T>::createBlock(long int capacityElements) {
    if ( capacityElements <= 0 ) {
        return NULL;
    }
    Block *block = new Block();
    if ( block == NULL ) {
        return NULL;
    }
    block->buffer = new char[capacityElements * ((long int)sizeof(T))];
    if ( block->buffer == NULL ) {
        delete block;
        return NULL;
    }
    block->capacityElements = capacityElements;
    block->usedElements = 0;
    block->prev = NULL;
    block->next = NULL;
    return block;
}

template <class T>
void
MemoryPool<T>::destroyBlock(Block *block) {
    if ( block == NULL ) {
        return;
    }
    delete[] block->buffer;
    block->buffer = NULL;
    delete block;
}

template <class T>
void
MemoryPool<T>::init(long int sizeInBytes) {
    if ( initialized || sizeInBytes <= 0 ) {
        return;
    }

    long int elements = sizeInBytes / ((long int)(sizeof(T)));
    if ( elements <= 0 ) {
        elements = 1;
    }

    Block *first = createBlock(elements);
    if ( first == NULL ) {
        return;
    }

    head = first;
    tail = first;
    current = first;
    initialized = true;
}

template <class T>
T *
MemoryPool<T>::allocate(int numberOfElements) {
    if ( !initialized || numberOfElements <= 0 ) {
        return NULL;
    }

    const long int request = (long int)numberOfElements;
    Block *scan = current != NULL ? current : tail;
    while ( scan != NULL ) {
        if ( scan->usedElements + request <= scan->capacityElements ) {
            T *out = (T *)(
                scan->buffer + (scan->usedElements * ((long int)sizeof(T))));
            scan->usedElements += request;
            current = scan;
            return out;
        }
        scan = scan->next;
    }

    return NULL;
}

template <class T>
void
MemoryPool<T>::free(int numberOfElements) {
    if ( !initialized || numberOfElements <= 0 ) {
        return;
    }

    long int remaining = ((long int)(numberOfElements));
    Block *scan = current != NULL ? current : tail;
    while ( scan != NULL && remaining > 0 ) {
        if ( scan->usedElements >= remaining ) {
            scan->usedElements -= remaining;
            remaining = 0;
            current = scan;
        } else {
            remaining -= scan->usedElements;
            scan->usedElements = 0;
            scan = scan->prev;
            current = scan;
        }
    }

    if ( current == NULL ) {
        current = head;
    }
}

template <class T>
void
MemoryPool<T>::clear() {
    for ( Block *it = head; it != NULL; it = it->next ) {
        it->usedElements = 0;
    }
    current = head;
}

template <class T>
bool
MemoryPool<T>::expand(int numberOfElements) {
    if ( numberOfElements <= 0 ) {
        return true;
    }

    Block *newBlock = createBlock((long int)numberOfElements);
    if ( newBlock == NULL ) {
        return false;
    }

    if ( head == NULL ) {
        head = newBlock;
        tail = newBlock;
        current = newBlock;
    } else {
        newBlock->prev = tail;
        tail->next = newBlock;
        tail = newBlock;
        current = newBlock;
    }
    initialized = true;
    return true;
}

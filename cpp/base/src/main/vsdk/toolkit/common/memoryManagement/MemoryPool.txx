#include "vsdk/toolkit/common/memoryManagement/MemoryPool.h"

template <class T>
MemoryPool<T>::MemoryPool():
    head(nullptr),
    tail(nullptr),
    current(nullptr),
    initialized(false)
{
}

template <class T>
MemoryPool<T>::~MemoryPool() {
    Block *it = head;
    while ( it != nullptr ) {
        Block *next = it->next;
        destroyBlock(it);
        it = next;
    }
    head = nullptr;
    tail = nullptr;
    current = nullptr;
    initialized = false;
}

template <class T>
typename MemoryPool<T>::Block *
MemoryPool<T>::createBlock(long int capacityElements) {
    if ( capacityElements <= 0 ) {
        return nullptr;
    }
    Block *block = new Block();
    if ( block == nullptr ) {
        return nullptr;
    }
    block->buffer = new char[capacityElements * static_cast<long int>(sizeof(T))];
    if ( block->buffer == nullptr ) {
        delete block;
        return nullptr;
    }
    block->capacityElements = capacityElements;
    block->usedElements = 0;
    block->prev = nullptr;
    block->next = nullptr;
    return block;
}

template <class T>
void
MemoryPool<T>::destroyBlock(Block *block) {
    if ( block == nullptr ) {
        return;
    }
    delete[] block->buffer;
    block->buffer = nullptr;
    delete block;
}

template <class T>
void
MemoryPool<T>::init(long int sizeInBytes) {
    if ( initialized ) {
        return;
    }

    if ( sizeInBytes <= 0 ) {
        return;
    }

    long int elements = sizeInBytes / static_cast<long int>(sizeof(T));
    if ( elements <= 0 ) {
        elements = 1;
    }

    Block *first = createBlock(elements);
    if ( first == nullptr ) {
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
        return nullptr;
    }

    const long int request = static_cast<long int>(numberOfElements);
    if ( request <= 0 ) {
        return nullptr;
    }

    Block *scan = current != nullptr ? current : tail;
    while ( scan != nullptr ) {
        if ( scan->usedElements + request <= scan->capacityElements ) {
            T *out = reinterpret_cast<T *>(
                scan->buffer + (scan->usedElements * static_cast<long int>(sizeof(T))));
            scan->usedElements += request;
            current = scan;
            return out;
        }
        scan = scan->next;
    }

    return nullptr;
}

template <class T>
void
MemoryPool<T>::free(int numberOfElements) {
    if ( !initialized || numberOfElements <= 0 ) {
        return;
    }

    const long int release = static_cast<long int>(numberOfElements);
    long int remaining = release;
    Block *scan = current != nullptr ? current : tail;
    while ( scan != nullptr && remaining > 0 ) {
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

    if ( current == nullptr ) {
        current = head;
    }
}

template <class T>
void
MemoryPool<T>::clear() {
    for ( Block *it = head; it != nullptr; it = it->next ) {
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

    const long int grow = static_cast<long int>(numberOfElements);
    if ( grow <= 0 ) {
        return false;
    }

    Block *newBlock = createBlock(grow);
    if ( newBlock == nullptr ) {
        return false;
    }

    if ( head == nullptr ) {
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

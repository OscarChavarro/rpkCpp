#ifndef __CIRCULAR_ARRAY_LIST__
#define __CIRCULAR_ARRAY_LIST__

#include <type_traits>

#include "java/lang/IllegalArgumentException.h"
#include "java/lang/IndexOutOfBoundsException.h"
#include "java/lang/Object.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/common/dataStructures/ArrayLinkable.h"
#include "vsdk/toolkit/common/logging/Logger.h"

/**
C++ port note: the Java `elements[i] instanceof ArrayLinkable` test is
resolved at compile time. Only an element type that is a pointer to a class
derived from `ArrayLinkable` keeps a handle; any other element type (as the
`String` elements of the tests) is always found by a scan.
*/
template <class E, bool LINKABLE =
    std::is_pointer<E>::value &&
    std::is_base_of<ArrayLinkable,
        typename std::remove_cv<
            typename std::remove_pointer<E>::type>::type>::value>
class _CircularArrayListHandle
{
public:
    static ArrayLinkable* of(const E& e)
    {
        (void)e;
        return nullptr;
    }
};

template <class E>
class _CircularArrayListHandle<E, true>
{
public:
    static ArrayLinkable* of(const E& e)
    {
        return e;
    }
};

/**
Circular list kept in an array. It backs every face, loop, edge, vertex and
half-edge collection of PolyhedralBoundedSolid, in place of the intrusive
nxt/prv pointers of [MANT1988] Ch. 10.2-10.3 and the list procedures of
Ch. 11.2 (addlist, dellist).

It offers the operations of the linked list it replaces
(`CircularDoubleLinkedList`), with the same order of elements and the same
preconditions, and differs in two ways that matter to the kernel:
- Reading does not write. `get`, `size`, `contains`, `nextOf` and
  `previousOf` change no field of the list, so several threads may read
  a list that nobody is editing. The linked list kept an access memo
  that `get` updated, which made concurrent readers race.
- `get` is O(1). Insertions and removals in the middle move the tail of
  the array, O(n).

The cursor operations (`next`, `previous`, `getWindow`, the `locate*`
methods and `removeElemAtWindow`) do write the cursor: they are for the
code that edits the list, on one thread.

C++ port notes: elements are compared with `==`, which is the Java identity
comparison for the pointer elements of the kernel. "null" is `E()`. The list
can not be copied, since its elements keep handles to it.
*/
template <class E>
class CircularArrayList : public java::Object
{
private:
    static const int NO_WINDOW = -1;

    E* elements;
    int capacity;
    int currentSize;
    /// Index of the cursor, or NO_WINDOW.
    int window;

    CircularArrayList(const CircularArrayList& other);
    CircularArrayList& operator=(const CircularArrayList& other);

    E at(int index) const
    {
        return elements[index];
    }

    /** Records in `elements[index]`, when it keeps a handle, where it is. */
    void attach(int index)
    {
        ArrayLinkable* handle =
            _CircularArrayListHandle<E>::of(elements[index]);
        if ( handle != nullptr ) {
            handle->setListSlot(this, index);
        }
    }

    void attachFrom(int first)
    {
        int i;

        for ( i = first; i < currentSize; i++ ) {
            attach(i);
        }
    }

    /** Clears the handle of `e` when it still points to this list. */
    void detach(const E& e)
    {
        ArrayLinkable* handle = _CircularArrayListHandle<E>::of(e);
        if ( handle != nullptr && handle->getListOwner() == this ) {
            handle->setListSlot(nullptr, NO_WINDOW);
        }
    }

    void insertAt(int index, const E& e)
    {
        int i;
        // Copy first: `e` may refer to a slot of `elements`, freed on growth.
        E value = e;

        if ( currentSize >= capacity ) {
            // Growth copies around the gap at `index` in a single pass, so
            // no shift is done on the new array (that shift made GCC report
            // a false -Wstringop-overflow on `new E[2 * capacity]`).
            int newCapacity = (capacity > 0) ? 2 * capacity : 8;
            E* grown = new E[newCapacity];
            for ( i = 0; i < index; i++ ) {
                grown[i] = elements[i];
            }
            for ( i = index; i < currentSize; i++ ) {
                grown[i + 1] = elements[i];
            }
            grown[index] = value;
            delete[] elements;
            elements = grown;
            capacity = newCapacity;
        }
        else {
            for ( i = currentSize; i > index; i-- ) {
                elements[i] = elements[i - 1];
            }
            elements[index] = value;
        }
        currentSize++;
        attachFrom(index);
    }

    void removeAt(int index)
    {
        E removed = elements[index];
        int i;

        for ( i = index; i < currentSize - 1; i++ ) {
            elements[i] = elements[i + 1];
        }
        currentSize--;
        elements[currentSize] = E();
        detach(removed);
        attachFrom(index);
    }

public:
    CircularArrayList()
    {
        elements = new E[8];
        capacity = 8;
        currentSize = 0;
        window = NO_WINDOW;
    }

    virtual ~CircularArrayList()
    {
        delete[] elements;
    }

    /**
    Index of `e` (compared by identity), or -1. O(1) when `e` keeps a handle
    that points to this list; a scan otherwise. Reads only.
    */
    int indexOf(const E& e) const
    {
        int i;
        ArrayLinkable* handle = _CircularArrayListHandle<E>::of(e);

        if ( handle != nullptr ) {
            int index = handle->getListIndex();
            if ( handle->getListOwner() == this && index >= 0 &&
                 index < currentSize && elements[index] == e ) {
                return index;
            }
        }
        for ( i = 0; i < currentSize; i++ ) {
            if ( elements[i] == e ) {
                return i;
            }
        }
        return -1;
    }

    bool contains(const E& e) const
    {
        return indexOf(e) >= 0;
    }

    int size() const
    {
        return currentSize;
    }

    /** Appends `e` after the last element. The cursor does not move. */
    void add(const E& e)
    {
        insertAt(currentSize, e);
    }

    /**
    Inserts `newElem` immediately before `pivot` and leaves the cursor on
    `pivot`. When `pivot` is the first element, `newElem` becomes the first.
    A pivot that is not in the list is a broken precondition (defect L-03 in
    Fable5.2Analysis.md): the list is left unchanged and an
    IllegalArgumentException is thrown.
    */
    void insertBefore(const E& newElem, const E& pivot)
    {
        int pivotIndex = indexOf(pivot);

        if ( pivotIndex < 0 ) {
            throw java::IllegalArgumentException(
                "CircularArrayList.insertBefore: pivot is not in the list");
        }
        insertAt(pivotIndex, newElem);
        window = pivotIndex + 1;
    }

    /** Moves the cursor to `index`; an index out of range leaves it alone. */
    void locateWindowAtIndex(int index)
    {
        if ( index < 0 || index >= currentSize ) {
            return;
        }
        window = index;
    }

    /**
    Moves the cursor to `e`. When `e` is not in the list the cursor is
    cleared and false is returned.
    */
    bool locateWindowAtElem(const E& e)
    {
        window = indexOf(e);
        return window >= 0;
    }

    /** Exchanges two elements; the cursor is left on the place of `e2`. */
    void swapElements(const E& e1, const E& e2)
    {
        int index1 = indexOf(e1);
        int index2 = indexOf(e2);

        window = index2;
        if ( index1 < 0 || index2 < 0 ) {
            return;
        }
        E temp = elements[index1];
        elements[index1] = elements[index2];
        elements[index2] = temp;
        attach(index1);
        attach(index2);
    }

    /**
    Returns the element under the cursor and advances the cursor, wrapping
    around. A cleared cursor starts at the first element.
    */
    E next()
    {
        if ( currentSize == 0 ) {
            return E();
        }
        if ( window == NO_WINDOW ) {
            window = 0;
        }
        E elem = at(window);
        window = (window + 1) % currentSize;
        return elem;
    }

    E getWindow()
    {
        if ( currentSize == 0 ) {
            return E();
        }
        if ( window == NO_WINDOW ) {
            window = 0;
        }
        return at(window);
    }

    /**
    Returns the element under the cursor and moves the cursor back, wrapping
    around. A cleared cursor starts at the first element.
    */
    E previous()
    {
        if ( currentSize == 0 ) {
            return E();
        }
        if ( window == NO_WINDOW ) {
            window = 0;
        }
        E elem = at(window);
        window = (window + currentSize - 1) % currentSize;
        return elem;
    }

    /** Element after `e`, circularly; null when `e` is not in the list. */
    E nextOf(const E& e) const
    {
        int index = indexOf(e);

        return index < 0 ? E() : at((index + 1) % currentSize);
    }

    /** Element before `e`, circularly; null when `e` is not in the list. */
    E previousOf(const E& e) const
    {
        int index = indexOf(e);

        return index < 0 ? E() : at((index + currentSize - 1) % currentSize);
    }

    E get(int index) const
    {
        if ( index < 0 || index >= currentSize ) {
            java::String msg;
            msg = java::String("IndexOutOfBounds Exception! - Trying to `get` with index ") + java::String::valueOf(index) + " in a list with " + java::String::valueOf(currentSize) + " elements.";

            Logger::reportMessage("CircularArrayList", Logger::FATAL_ERROR, "get", msg);
            return E();
        }
        return at(index);
    }

    /**
    Removes the element at `pos` and clears the cursor. An index outside
    [0, size) is a broken precondition (defect L-02 in Fable5.2Analysis.md):
    the list is left unchanged and an IndexOutOfBoundsException is thrown.
    */
    void remove(int pos)
    {
        if ( pos < 0 || pos >= currentSize ) {
            throw java::IndexOutOfBoundsException(
                java::String("CircularArrayList.remove: index ") +
                java::String::valueOf(pos) +
                " in a list with " + java::String::valueOf(currentSize) +
                " elements");
        }
        removeAt(pos);
        window = NO_WINDOW;
    }

    /** Removes the element under the cursor, if any, and clears the cursor. */
    void removeElemAtWindow()
    {
        if ( window == NO_WINDOW ) {
            return;
        }
        removeAt(window);
        window = NO_WINDOW;
    }

    /** Removes every element. */
    void clear()
    {
        int i;

        for ( i = 0; i < currentSize; i++ ) {
            detach(elements[i]);
            elements[i] = E();
        }
        currentSize = 0;
        window = NO_WINDOW;
    }

    /**
    Inserts `newElem` as the first element. The cursor is left on the
    element that was first, or cleared when the list was empty.
    */
    void push(const E& newElem)
    {
        bool wasEmpty = currentSize == 0;

        insertAt(0, newElem);
        window = wasEmpty ? NO_WINDOW : 1;
    }

    /** Reverses the order of the elements. The cursor keeps its index. */
    void reverse()
    {
        int i;
        int j;

        for ( i = 0, j = currentSize - 1; i < j; i++, j-- ) {
            E temp = elements[i];
            elements[i] = elements[j];
            elements[j] = temp;
        }
        attachFrom(0);
    }
};

#endif

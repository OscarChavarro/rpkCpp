#ifndef __CIRCULAR_ARRAY_LIST__
#define __CIRCULAR_ARRAY_LIST__

#include <stddef.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/IllegalArgumentException.h"
#include "java/lang/IndexOutOfBoundsException.h"
#include "java/lang/Object.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/common/Cpp98Support.h"
#include "vsdk/toolkit/common/dataStructures/ArrayLinkable.h"
#include "vsdk/toolkit/common/logging/Logger.h"

/**
Handle of an element of `CircularArrayList`: the Java
`elements[i] instanceof ArrayLinkable` test, resolved by overloading. A
pointer to a class derived from `ArrayLinkable` takes the first overload (a
conversion to a base class is preferred to an ellipsis); any other
pointer, and the `String` elements of the tests, have no handle and are
always found by a scan.

C++ port note: the element class must be complete where the list members
are compiled, or its pointers take the ellipsis overload. The headers that
declare lists of nodes include the node headers for that reason; the
handle is only a shortcut, so correctness never depends on it.
*/
inline ArrayLinkable*
circularArrayListHandle(ArrayLinkable* e)
{
    return e;
}

/*
The fallback takes an ellipsis, not a `const void *`: Turbo C++ 3.0 ranks the
conversions of a derived pointer to its base and to `void *` equally (an
ambiguous call), while an ellipsis match is the worst one for every
compiler. The elements are pointers or `String`, so nothing that is not
trivially copyable goes through it.
*/
inline ArrayLinkable*
circularArrayListHandle(...)
{
    return NULL;
}

inline ArrayLinkable*
circularArrayListHandle(const String& /*e*/)
{
    return NULL;
}

/**
@return a new array of `n` elements. The first argument only selects `E`
(legacy compilers lack explicit template arguments for functions).
C++ port note: gcc 2.7.2 crashes on members of class templates that store
the result of `new E[]`; the list allocates through this function.
*/
template <class E> E*
newCircularArrayListStorage(E* /*selector*/, long n)
{
    return new E[n];
}

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
comparison for the pointer elements of the kernel. "null" is the default
value of `E`. Indexes and sizes are `long`, as in the port's `ArrayList`
(`int` has 16 bits on DOS). The storage is allocated on the first insertion
(Java allocates 8 slots at construction), as many loops and faces of the
port stay with few elements. The list can not be copied, since its
elements keep handles to it. Every member is defined in the class body (the
legacy compilers instantiate those reliably).
*/
template <class E>
class CircularArrayList : public Object
{
private:
    enum { NO_WINDOW = -1, INITIAL_CAPACITY = 8 };

    E* elements;
    long capacity;
    long currentSize;
    /// Index of the cursor, or NO_WINDOW.
    long window;

    CircularArrayList(const CircularArrayList& other);
    CircularArrayList& operator=(const CircularArrayList& other);

    E at(long index) const
    {
        return elements[index];
    }

    E nullElement() const
    {
        return vsdkDefaultValue((E*)NULL);
    }

    /** Records in `elements[index]`, when it keeps a handle, where it is. */
    void attach(long index)
    {
        ArrayLinkable* handle = circularArrayListHandle(elements[index]);

        if ( handle != NULL ) {
            handle->setListSlot(this, index);
        }
    }

    void attachFrom(long first)
    {
        long i;

        for ( i = first; i < currentSize; i++ ) {
            attach(i);
        }
    }

    /** Clears the handle of `e` when it still points to this list. */
    void detach(const E& e)
    {
        ArrayLinkable* handle = circularArrayListHandle(e);

        if ( handle != NULL && handle->getListOwner() == this ) {
            handle->setListSlot(NULL, NO_WINDOW);
        }
    }

    void insertAt(long index, const E& e)
    {
        long i;

        if ( currentSize == capacity ) {
            long newCapacity = capacity == 0 ? (long)INITIAL_CAPACITY :
                2 * capacity;
            E* newElements = newCircularArrayListStorage((E*)NULL,
                newCapacity);
            for ( i = 0; i < currentSize; i++ ) {
                newElements[i] = elements[i];
            }
            delete[] elements;
            elements = newElements;
            capacity = newCapacity;
        }
        for ( i = currentSize; i > index; i-- ) {
            elements[i] = elements[i - 1];
        }
        elements[index] = e;
        currentSize++;
        attachFrom(index);
    }

    void removeAt(long index)
    {
        E removed = elements[index];
        long i;

        for ( i = index; i < currentSize - 1; i++ ) {
            elements[i] = elements[i + 1];
        }
        currentSize--;
        elements[currentSize] = nullElement();
        detach(removed);
        attachFrom(index);
    }

public:
    CircularArrayList()
    {
        elements = NULL;
        capacity = 0;
        currentSize = 0;
        window = NO_WINDOW;
    }

    /**
    C++ port note: the handles of the elements are not cleared, as the
    owners of the elements usually delete them before the list. A stale
    handle is harmless: `indexOf` checks it against the list.
    */
    virtual ~CircularArrayList()
    {
        delete[] elements;
    }

    /**
    Index of `e` (compared by identity), or -1. O(1) when `e` keeps a handle
    that points to this list; a scan otherwise. Reads only.
    */
    long indexOf(const E& e) const
    {
        ArrayLinkable* handle = circularArrayListHandle(e);
        long i;

        if ( handle != NULL ) {
            long index = handle->getListIndex();
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

    BOOLEAN contains(const E& e) const
    {
        return indexOf(e) >= 0;
    }

    long size() const
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
        long pivotIndex = indexOf(pivot);

        if ( pivotIndex < 0 ) {
            throw IllegalArgumentException(
                "CircularArrayList.insertBefore: pivot is not in the list");
        }
        insertAt(pivotIndex, newElem);
        window = pivotIndex + 1;
    }

    /** Moves the cursor to `index`; an index out of range leaves it alone. */
    void locateWindowAtIndex(long index)
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
    BOOLEAN locateWindowAtElem(const E& e)
    {
        window = indexOf(e);
        return window >= 0;
    }

    /** Exchanges two elements; the cursor is left on the place of `e2`. */
    void swapElements(const E& e1, const E& e2)
    {
        long index1 = indexOf(e1);
        long index2 = indexOf(e2);

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
            return nullElement();
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
            return nullElement();
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
            return nullElement();
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
        long index = indexOf(e);

        if ( index < 0 ) {
            return nullElement();
        }
        return at((index + 1) % currentSize);
    }

    /** Element before `e`, circularly; null when `e` is not in the list. */
    E previousOf(const E& e) const
    {
        long index = indexOf(e);

        if ( index < 0 ) {
            return nullElement();
        }
        return at((index + currentSize - 1) % currentSize);
    }

    E get(long index) const
    {
        if ( index < 0 || index >= currentSize ) {
            String msg = String("IndexOutOfBounds Exception! - Trying to `get` with index ") +
                Cpp98Support::toString(index) + " in a list with " +
                Cpp98Support::toString(currentSize) + " elements.";
            Logger::reportMessage("CircularArrayList", Logger::FATAL_ERROR,
                "get", msg);
            return nullElement();
        }
        return at(index);
    }

    /**
    Removes the element at `pos` and clears the cursor. An index outside
    [0, size) is a broken precondition (defect L-02 in Fable5.2Analysis.md):
    the list is left unchanged and an IndexOutOfBoundsException is thrown.
    */
    void remove(long pos)
    {
        if ( pos < 0 || pos >= currentSize ) {
            String msg = String("CircularArrayList.remove: index ") +
                Cpp98Support::toString(pos) + " in a list with " +
                Cpp98Support::toString(currentSize) + " elements";
            throw IndexOutOfBoundsException(msg);
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
        long i;

        for ( i = 0; i < currentSize; i++ ) {
            detach(elements[i]);
            elements[i] = nullElement();
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
        BOOLEAN wasEmpty = currentSize == 0;

        insertAt(0, newElem);
        if ( wasEmpty ) {
            window = NO_WINDOW;
        }
        else {
            window = 1;
        }
    }

    /** Reverses the order of the elements. The cursor keeps its index. */
    void reverse()
    {
        long i;
        long j;

        for ( i = 0, j = currentSize - 1; i < j; i++, j-- ) {
            E temp = elements[i];
            elements[i] = elements[j];
            elements[j] = temp;
        }
        attachFrom(0);
    }
};

#endif

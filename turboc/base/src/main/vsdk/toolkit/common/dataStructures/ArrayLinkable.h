#ifndef __ARRAY_LINKABLE__
#define __ARRAY_LINKABLE__

#include <stddef.h>

/**
Optional element-side handle for `CircularArrayList`: an element that
implements it remembers the list that holds it and its index there, so
`contains`, `nextOf`, `previousOf` and `locateWindowAtElem` run in O(1)
instead of scanning the list. This is how the half-edges of a loop recover
the `nxt`/`prv` links of [MANT1988] Program 10.1 while keeping the container
operations used throughout the kernel.

The list keeps the handle up to date on every structural change. When an
element is inserted in a second list, the handle follows the newest list and
the first one falls back to a scan, so correctness never depends on it.

C++ port notes: the Java interface, that every node class implements with
the same two fields and three methods, is a base class that holds them (no
virtual functions, so the nodes do not grow a table pointer on 16 bit
targets). The owner, a Java `Object`, is an untyped pointer to the list, and
the index is a `long`, as the indexes of the port's lists.
*/
class ArrayLinkable
{
private:
    /// Place of this element in the list that holds it, kept up to date by
    /// the list.
    const void* listOwner;
    long listIndex;

public:
    ArrayLinkable() : listOwner(NULL), listIndex(-1) {}

    /** List that last recorded this element, or null. */
    const void* getListOwner() const { return listOwner; }

    /** Index recorded by `getListOwner()`; meaningless without it. */
    long getListIndex() const { return listIndex; }

    /** Called by the list only. */
    void setListSlot(const void* owner, long index)
    {
        listOwner = owner;
        listIndex = index;
    }
};

#endif

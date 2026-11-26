#ifndef __ARRAY_LINKABLE__
#define __ARRAY_LINKABLE__

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

C++ port note: the Java interface is an abstract class; the owner, a Java
`Object`, is an untyped pointer to the list.
*/
class ArrayLinkable
{
public:
    virtual ~ArrayLinkable() {}

    /** List that last recorded this element, or null. */
    virtual const void* getListOwner() const = 0;

    /** Index recorded by `getListOwner()`; meaningless without it. */
    virtual int getListIndex() const = 0;

    /** Called by the list only. */
    virtual void setListSlot(const void* owner, int index) = 0;
};

#endif

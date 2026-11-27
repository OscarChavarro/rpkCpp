package vsdk.toolkit.common.dataStructures;

import java.io.Serial;
import java.util.Arrays;
import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.logging.Logger;

/**
Circular list kept in an array. It backs every face, loop, edge, vertex and
half-edge collection of PolyhedralBoundedSolid, in place of the intrusive
nxt/prv pointers of [MANT1988] Ch. 10.2-10.3 and the list procedures of
Ch. 11.2 (addlist, dellist).

<p>It offers the operations of the linked list it replaces
(`CircularDoubleLinkedList`), with the same order of elements and the same
preconditions, and differs in two ways that matter to the kernel:</p>
<ul>
<li>Reading does not write. `get`, `size`, `contains`, `nextOf` and
    `previousOf` change no field of the list, so several threads may read
    a list that nobody is editing. The linked list kept an access memo
    that `get` updated, which made concurrent readers race.</li>
<li>`get` is O(1). Insertions and removals in the middle move the tail of
    the array, O(n).</li>
</ul>

<p>The cursor operations (`next`, `previous`, `getWindow`, the `locate*`
methods and `removeElemAtWindow`) do write the cursor: they are for the
code that edits the list, on one thread.</p>
*/
public class CircularArrayList<E> extends FundamentalEntity
{
    @Serial private static final long serialVersionUID = 20261116L;

    private static final int NO_WINDOW = -1;

    private Object[] elements;
    private int currentSize;
    /// Index of the cursor, or NO_WINDOW.
    private int window;

    public CircularArrayList()
    {
        elements = new Object[8];
        currentSize = 0;
        window = NO_WINDOW;
    }

    @SuppressWarnings("unchecked")
    private E at(int index)
    {
        return (E)elements[index];
    }

    /** Records in `elements[index]`, when it keeps a handle, where it is. */
    private void attach(int index)
    {
        if ( elements[index] instanceof ArrayLinkable ) {
            ((ArrayLinkable)elements[index]).setListSlot(this, index);
        }
    }

    private void attachFrom(int first)
    {
        int i;

        for ( i = first; i < currentSize; i++ ) {
            attach(i);
        }
    }

    /** Clears the handle of `e` when it still points to this list. */
    private void detach(Object e)
    {
        if ( e instanceof ArrayLinkable &&
             ((ArrayLinkable)e).getListOwner() == this ) {
            ((ArrayLinkable)e).setListSlot(null, NO_WINDOW);
        }
    }

    /**
    Index of `e` (compared by identity), or -1. O(1) when `e` keeps a handle
    that points to this list; a scan otherwise. Reads only.
    */
    public int indexOf(E e)
    {
        int i;

        if ( e instanceof ArrayLinkable ) {
            ArrayLinkable handle = (ArrayLinkable)e;
            int index = handle.getListIndex();
            if ( handle.getListOwner() == this && index >= 0 &&
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

    public boolean contains(E e)
    {
        return indexOf(e) >= 0;
    }

    public int size()
    {
        return currentSize;
    }

    private void insertAt(int index, E e)
    {
        if ( currentSize == elements.length ) {
            elements = Arrays.copyOf(elements, 2 * elements.length);
        }
        System.arraycopy(elements, index, elements, index + 1,
            currentSize - index);
        elements[index] = e;
        currentSize++;
        attachFrom(index);
    }

    private void removeAt(int index)
    {
        Object removed = elements[index];

        System.arraycopy(elements, index + 1, elements, index,
            currentSize - index - 1);
        currentSize--;
        elements[currentSize] = null;
        detach(removed);
        attachFrom(index);
    }

    /** Appends `e` after the last element. The cursor does not move. */
    public void add(E e)
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
    public void insertBefore(E newElem, E pivot)
    {
        int pivotIndex = indexOf(pivot);

        if ( pivotIndex < 0 ) {
            throw new IllegalArgumentException(
                "CircularArrayList.insertBefore: pivot is not in the list");
        }
        insertAt(pivotIndex, newElem);
        window = pivotIndex + 1;
    }

    /** Moves the cursor to `index`; an index out of range leaves it alone. */
    public void locateWindowAtIndex(int index)
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
    public boolean locateWindowAtElem(E e)
    {
        window = indexOf(e);
        return window >= 0;
    }

    /** Exchanges two elements; the cursor is left on the place of `e2`. */
    public void swapElements(E e1, E e2)
    {
        int index1 = indexOf(e1);
        int index2 = indexOf(e2);

        window = index2;
        if ( index1 < 0 || index2 < 0 ) {
            return;
        }
        Object temp = elements[index1];
        elements[index1] = elements[index2];
        elements[index2] = temp;
        attach(index1);
        attach(index2);
    }

    /**
    Returns the element under the cursor and advances the cursor, wrapping
    around. A cleared cursor starts at the first element.
    */
    public E next()
    {
        if ( currentSize == 0 ) {
            return null;
        }
        if ( window == NO_WINDOW ) {
            window = 0;
        }
        E elem = at(window);
        window = (window + 1) % currentSize;
        return elem;
    }

    public E getWindow()
    {
        if ( currentSize == 0 ) {
            return null;
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
    public E previous()
    {
        if ( currentSize == 0 ) {
            return null;
        }
        if ( window == NO_WINDOW ) {
            window = 0;
        }
        E elem = at(window);
        window = (window + currentSize - 1) % currentSize;
        return elem;
    }

    /** Element after `e`, circularly; null when `e` is not in the list. */
    public E nextOf(E e)
    {
        int index = indexOf(e);

        return index < 0 ? null : at((index + 1) % currentSize);
    }

    /** Element before `e`, circularly; null when `e` is not in the list. */
    public E previousOf(E e)
    {
        int index = indexOf(e);

        return index < 0 ? null : at((index + currentSize - 1) % currentSize);
    }

    public E get(int index)
    {
        if ( index < 0 || index >= currentSize ) {
            String msg;
            msg = "IndexOutOfBounds Exception! - Trying to `get` with index " + index + " in a list with " + currentSize + " elements.";

            Logger.reportMessage(this, VSDK.FATAL_ERROR, "get", msg);
            return null;
        }
        return at(index);
    }

    /**
    Removes the element at `pos` and clears the cursor. An index outside
    [0, size) is a broken precondition (defect L-02 in Fable5.2Analysis.md):
    the list is left unchanged and an IndexOutOfBoundsException is thrown.
    */
    public void remove(int pos)
    {
        if ( pos < 0 || pos >= currentSize ) {
            throw new IndexOutOfBoundsException(
                "CircularArrayList.remove: index " + pos +
                " in a list with " + currentSize + " elements");
        }
        removeAt(pos);
        window = NO_WINDOW;
    }

    /** Removes the element under the cursor, if any, and clears the cursor. */
    public void removeElemAtWindow()
    {
        if ( window == NO_WINDOW ) {
            return;
        }
        removeAt(window);
        window = NO_WINDOW;
    }

    /** Removes every element. */
    public void clear()
    {
        int i;

        for ( i = 0; i < currentSize; i++ ) {
            detach(elements[i]);
            elements[i] = null;
        }
        currentSize = 0;
        window = NO_WINDOW;
    }

    /**
    Inserts `newElem` as the first element. The cursor is left on the
    element that was first, or cleared when the list was empty.
    */
    public void push(E newElem)
    {
        boolean wasEmpty = currentSize == 0;

        insertAt(0, newElem);
        window = wasEmpty ? NO_WINDOW : 1;
    }

    /** Reverses the order of the elements. The cursor keeps its index. */
    public void reverse()
    {
        int i;
        int j;

        for ( i = 0, j = currentSize - 1; i < j; i++, j-- ) {
            Object temp = elements[i];
            elements[i] = elements[j];
            elements[j] = temp;
        }
        attachFrom(0);
    }
}

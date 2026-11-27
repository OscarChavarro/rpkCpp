package vsdk.toolkit.common.dataStructures;

/**
Optional element-side handle for {@link CircularArrayList}: an element that
implements it remembers the list that holds it and its index there, so
`contains`, `nextOf`, `previousOf` and `locateWindowAtElem` run in O(1)
instead of scanning the list. This is how the half-edges of a loop recover
the `nxt`/`prv` links of [MANT1988] Program 10.1 while keeping the container
operations used throughout the kernel.

<p>The list keeps the handle up to date on every structural change. When an
element is inserted in a second list, the handle follows the newest list and
the first one falls back to a scan, so correctness never depends on it.</p>
*/
public interface ArrayLinkable
{
    /** List that last recorded this element, or null. */
    Object getListOwner();

    /** Index recorded by {@link #getListOwner()}; meaningless without it. */
    int getListIndex();

    /** Called by the list only. */
    void setListSlot(Object owner, int index);
}

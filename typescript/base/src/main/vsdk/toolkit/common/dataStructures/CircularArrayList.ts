import { IllegalArgumentException } from "../../../../java/lang/IllegalArgumentException.js";
import { IndexOutOfBoundsException } from "../../../../java/lang/IndexOutOfBoundsException.js";
import { FundamentalEntity } from "../FundamentalEntity.js";
import { VSDK } from "../VSDK.js";
import { Logger } from "../logging/Logger.js";
import { isArrayLinkable } from "./ArrayLinkable.js";

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
export class CircularArrayList<E> extends FundamentalEntity {
    private static readonly NO_WINDOW = -1;

    private elements: (E | null)[];
    private currentSize: number;
    /// Index of the cursor, or NO_WINDOW.
    private window: number;

    public constructor() {
        super();
        this.elements = new Array<E | null>(8).fill(null);
        this.currentSize = 0;
        this.window = CircularArrayList.NO_WINDOW;
    }

    private at(index: number): E {
        return this.elements[index] as E;
    }

    /** Records in `elements[index]`, when it keeps a handle, where it is. */
    private attach(index: number): void {
        const e = this.elements[index];
        if (isArrayLinkable(e)) {
            e.setListSlot(this, index);
        }
    }

    private attachFrom(first: number): void {
        let i: number;

        for (i = first; i < this.currentSize; i++) {
            this.attach(i);
        }
    }

    /** Clears the handle of `e` when it still points to this list. */
    private detach(e: unknown): void {
        if (isArrayLinkable(e) && e.getListOwner() === this) {
            e.setListSlot(null, CircularArrayList.NO_WINDOW);
        }
    }

    /**
    Index of `e` (compared by identity), or -1. O(1) when `e` keeps a handle
    that points to this list; a scan otherwise. Reads only.
    */
    public indexOf(e: E): number {
        let i: number;

        if (isArrayLinkable(e)) {
            const handle = e;
            const index = handle.getListIndex();
            if (
                handle.getListOwner() === this &&
                index >= 0 &&
                index < this.currentSize &&
                this.elements[index] === e
            ) {
                return index;
            }
        }
        for (i = 0; i < this.currentSize; i++) {
            if (this.elements[i] === e) {
                return i;
            }
        }
        return -1;
    }

    public contains(e: E): boolean {
        return this.indexOf(e) >= 0;
    }

    public size(): number {
        return this.currentSize;
    }

    private insertAt(index: number, e: E): void {
        if (this.currentSize === this.elements.length) {
            const grown = new Array<E | null>(2 * this.elements.length).fill(null);
            for (let k = 0; k < this.currentSize; k++) {
                grown[k] = this.elements[k] ?? null;
            }
            this.elements = grown;
        }
        this.elements.copyWithin(index + 1, index, this.currentSize);
        this.elements[index] = e;
        this.currentSize++;
        this.attachFrom(index);
    }

    private removeAt(index: number): void {
        const removed = this.elements[index];

        this.elements.copyWithin(index, index + 1, this.currentSize);
        this.currentSize--;
        this.elements[this.currentSize] = null;
        this.detach(removed);
        this.attachFrom(index);
    }

    /** Appends `e` after the last element. The cursor does not move. */
    public add(e: E): void {
        this.insertAt(this.currentSize, e);
    }

    /**
    Inserts `newElem` immediately before `pivot` and leaves the cursor on
    `pivot`. When `pivot` is the first element, `newElem` becomes the first.
    A pivot that is not in the list is a broken precondition (defect L-03 in
    Fable5.2Analysis.md): the list is left unchanged and an
    IllegalArgumentException is thrown.
    */
    public insertBefore(newElem: E, pivot: E): void {
        const pivotIndex = this.indexOf(pivot);

        if (pivotIndex < 0) {
            throw new IllegalArgumentException("CircularArrayList.insertBefore: pivot is not in the list");
        }
        this.insertAt(pivotIndex, newElem);
        this.window = pivotIndex + 1;
    }

    /** Moves the cursor to `index`; an index out of range leaves it alone. */
    public locateWindowAtIndex(index: number): void {
        if (index < 0 || index >= this.currentSize) {
            return;
        }
        this.window = index;
    }

    /**
    Moves the cursor to `e`. When `e` is not in the list the cursor is
    cleared and false is returned.
    */
    public locateWindowAtElem(e: E): boolean {
        this.window = this.indexOf(e);
        return this.window >= 0;
    }

    /** Exchanges two elements; the cursor is left on the place of `e2`. */
    public swapElements(e1: E, e2: E): void {
        const index1 = this.indexOf(e1);
        const index2 = this.indexOf(e2);

        this.window = index2;
        if (index1 < 0 || index2 < 0) {
            return;
        }
        const temp = this.elements[index1]!;
        this.elements[index1] = this.elements[index2]!;
        this.elements[index2] = temp;
        this.attach(index1);
        this.attach(index2);
    }

    /**
    Returns the element under the cursor and advances the cursor, wrapping
    around. A cleared cursor starts at the first element.
    */
    public next(): E | null {
        if (this.currentSize === 0) {
            return null;
        }
        if (this.window === CircularArrayList.NO_WINDOW) {
            this.window = 0;
        }
        const elem = this.at(this.window);
        this.window = (this.window + 1) % this.currentSize;
        return elem;
    }

    public getWindow(): E | null {
        if (this.currentSize === 0) {
            return null;
        }
        if (this.window === CircularArrayList.NO_WINDOW) {
            this.window = 0;
        }
        return this.at(this.window);
    }

    /**
    Returns the element under the cursor and moves the cursor back, wrapping
    around. A cleared cursor starts at the first element.
    */
    public previous(): E | null {
        if (this.currentSize === 0) {
            return null;
        }
        if (this.window === CircularArrayList.NO_WINDOW) {
            this.window = 0;
        }
        const elem = this.at(this.window);
        this.window = (this.window + this.currentSize - 1) % this.currentSize;
        return elem;
    }

    /** Element after `e`, circularly; null when `e` is not in the list. */
    public nextOf(e: E): E | null {
        const index = this.indexOf(e);

        return index < 0 ? null : this.at((index + 1) % this.currentSize);
    }

    /** Element before `e`, circularly; null when `e` is not in the list. */
    public previousOf(e: E): E | null {
        const index = this.indexOf(e);

        return index < 0 ? null : this.at((index + this.currentSize - 1) % this.currentSize);
    }

    public get(index: number): E | null {
        if (!Number.isInteger(index) || index < 0 || index >= this.currentSize) {
            let msg: string;
            msg =
                "IndexOutOfBounds Exception! - Trying to `get` with index " +
                index +
                " in a list with " +
                this.currentSize +
                " elements.";

            Logger.reportMessage(this, VSDK.FATAL_ERROR, "get", msg);
            return null;
        }
        return this.at(index);
    }

    /**
    Removes the element at `pos` and clears the cursor. An index outside
    [0, size) is a broken precondition (defect L-02 in Fable5.2Analysis.md):
    the list is left unchanged and an IndexOutOfBoundsException is thrown.
    */
    public remove(pos: number): void {
        if (!Number.isInteger(pos) || pos < 0 || pos >= this.currentSize) {
            throw new IndexOutOfBoundsException(
                "CircularArrayList.remove: index " + pos + " in a list with " + this.currentSize + " elements",
            );
        }
        this.removeAt(pos);
        this.window = CircularArrayList.NO_WINDOW;
    }

    /** Removes the element under the cursor, if any, and clears the cursor. */
    public removeElemAtWindow(): void {
        if (this.window === CircularArrayList.NO_WINDOW) {
            return;
        }
        this.removeAt(this.window);
        this.window = CircularArrayList.NO_WINDOW;
    }

    /** Removes every element. */
    public clear(): void {
        let i: number;

        for (i = 0; i < this.currentSize; i++) {
            this.detach(this.elements[i]);
            this.elements[i] = null;
        }
        this.currentSize = 0;
        this.window = CircularArrayList.NO_WINDOW;
    }

    /**
    Inserts `newElem` as the first element. The cursor is left on the
    element that was first, or cleared when the list was empty.
    */
    public push(newElem: E): void {
        const wasEmpty = this.currentSize === 0;

        this.insertAt(0, newElem);
        this.window = wasEmpty ? CircularArrayList.NO_WINDOW : 1;
    }

    /** Reverses the order of the elements. The cursor keeps its index. */
    public reverse(): void {
        let i: number;
        let j: number;

        for (i = 0, j = this.currentSize - 1; i < j; i++, j--) {
            const temp = this.elements[i]!;
            this.elements[i] = this.elements[j]!;
            this.elements[j] = temp;
        }
        this.attachFrom(0);
    }
}

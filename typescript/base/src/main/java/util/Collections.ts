import { UnsupportedOperationException } from "../lang/UnsupportedOperationException.js";
import { Double } from "../lang/Double.js";
import { ArrayList } from "./ArrayList.js";
import type { Comparator } from "./Comparator.js";
import { type Iterator } from "./Iterator.js";
import { TimSort } from "./TimSort.js";

/**
Subset of `java.util.Collections` needed by ported algorithms whose output
depends on Java's exact sorting and reversal semantics.
*/
export class Collections {
    private constructor() {}

    /**
    `Collections.sort(List)` and `Collections.sort(List, Comparator)`: a
    stable sort with Java's TimSort comparison sequence. Without a
    comparator, elements are compared by their natural ordering: numbers as
    `Double.compareTo`, strings as `String.compareTo`, and other objects by
    their `compareTo` method.
    */
    public static sort<T>(list: T[] | ArrayList<T>, c?: Comparator<T> | null): void {
        const compare =
            c === undefined || c === null
                ? (left: T, right: T): number => Collections.naturalCompare(left, right)
                : (left: T, right: T): number => c.compare(left, right);
        if (list instanceof ArrayList) {
            const elements = list.toArray();
            TimSort.sort(elements, 0, elements.length, compare);
            for (let i = 0; i < elements.length; i++) {
                list.set(i, elements[i]!);
            }
            return;
        }
        TimSort.sort(list, 0, list.length, compare);
    }

    /** `Collections.reverse(List)`. */
    public static reverse<T>(list: T[] | ArrayList<T>): void {
        const size = list instanceof ArrayList ? list.size() : list.length;
        for (let i = 0, mid = size >> 1, j = size - 1; i < mid; i++, j--) {
            if (list instanceof ArrayList) {
                list.set(i, list.set(j, list.get(i)));
            } else {
                const tmp = list[i]!;
                list[i] = list[j]!;
                list[j] = tmp;
            }
        }
    }

    /**
    `Collections.unmodifiableList(List)`: a read-only view over the given
    list whose mutator methods throw `UnsupportedOperationException`.
    */
    public static unmodifiableList<T>(list: ArrayList<T>): ArrayList<T> {
        return new UnmodifiableArrayList<T>(list);
    }

    private static naturalCompare<T>(left: T, right: T): number {
        if (typeof left === "number" && typeof right === "number") {
            return Double.compare(left, right);
        }
        if (typeof left === "string" && typeof right === "string") {
            const n = Math.min(left.length, right.length);
            for (let i = 0; i < n; i++) {
                const d = left.charCodeAt(i) - right.charCodeAt(i);
                if (d !== 0) {
                    return d;
                }
            }
            return left.length - right.length;
        }
        return (left as unknown as { compareTo(other: T): number }).compareTo(right);
    }
}

/**
Read-only view returned by {@link Collections.unmodifiableList}. Reads are
delegated to the backing list, so later changes made through the backing list
remain visible, and every mutator throws `UnsupportedOperationException`.
*/
class UnmodifiableArrayList<T> extends ArrayList<T> {
    private readonly backing: ArrayList<T>;

    public constructor(backing: ArrayList<T>) {
        super(0);
        this.backing = backing;
    }

    public override size(): number {
        return this.backing.size();
    }
    public override isEmpty(): boolean {
        return this.backing.isEmpty();
    }
    public override contains(value: T): boolean {
        return this.backing.contains(value);
    }
    public override get(index: number): T {
        return this.backing.get(index);
    }
    public override indexOf(value: T): number {
        return this.backing.indexOf(value);
    }
    public override lastIndexOf(value: T): number {
        return this.backing.lastIndexOf(value);
    }
    public override toArray(): T[] {
        return this.backing.toArray();
    }

    public override set(_index: number, _value: T): T {
        throw new UnsupportedOperationException();
    }
    public override add(value: T): boolean;
    public override add(index: number, value: T): void;
    public override add(_indexOrValue: number | T, _value?: T): boolean | void {
        throw new UnsupportedOperationException();
    }
    public override addAll(values: Iterable<T>): boolean;
    public override addAll(index: number, values: Iterable<T>): boolean;
    public override addAll(_indexOrValues: number | Iterable<T>, _values?: Iterable<T>): boolean {
        throw new UnsupportedOperationException();
    }
    public override remove(_index: number): T {
        throw new UnsupportedOperationException();
    }
    public override removeAt(_index: number): T {
        throw new UnsupportedOperationException();
    }
    public override removeElement(_value: T): boolean {
        throw new UnsupportedOperationException();
    }
    public override clear(): void {
        throw new UnsupportedOperationException();
    }

    public override iterator(): Iterator<T> {
        const delegate = this.backing.iterator();
        return {
            hasNext: (): boolean => delegate.hasNext(),
            next: (): T => delegate.next(),
            remove: (): void => {
                throw new UnsupportedOperationException();
            },
        };
    }

    public override *[Symbol.iterator](): IterableIterator<T> {
        const iterator = this.iterator();
        while (iterator.hasNext()) yield iterator.next();
    }
}

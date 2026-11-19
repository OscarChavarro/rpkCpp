import { IllegalArgumentException } from "../lang/IllegalArgumentException.js";
import { IllegalStateException } from "../lang/IllegalStateException.js";
import { IndexOutOfBoundsException } from "../lang/IndexOutOfBoundsException.js";
import { type Iterator } from "./Iterator.js";

/** Array-backed Java List with index checks and fail-fast iterators. */
export class ArrayList<T> implements Iterable<T> {
    private readonly elements: T[];
    private modificationCount = 0;

    public constructor(initialCapacityOrValues: number | Iterable<T> = 10) {
        if (typeof initialCapacityOrValues === "number") {
            if (!Number.isInteger(initialCapacityOrValues) || initialCapacityOrValues < 0)
                throw new IllegalArgumentException("Illegal Capacity");
            this.elements = [];
        } else this.elements = Array.from(initialCapacityOrValues);
    }

    public size(): number {
        return this.elements.length;
    }
    public isEmpty(): boolean {
        return this.elements.length === 0;
    }
    public contains(value: T): boolean {
        return this.indexOf(value) >= 0;
    }
    public get(index: number): T {
        this.requireElementIndex(index);
        return this.elements[index] as T;
    }
    public set(index: number, value: T): T {
        this.requireElementIndex(index);
        const previous = this.elements[index] as T;
        this.elements[index] = value;
        return previous;
    }

    public add(value: T): boolean;
    public add(index: number, value: T): void;
    public add(indexOrValue: number | T, value?: T): boolean | void {
        if (arguments.length === 2) {
            const index = indexOrValue as number;
            this.requirePositionIndex(index);
            this.elements.splice(index, 0, value as T);
            this.modificationCount++;
            return;
        }
        this.elements.push(indexOrValue as T);
        this.modificationCount++;
        return true;
    }

    public addAll(values: Iterable<T>): boolean;
    public addAll(index: number, values: Iterable<T>): boolean;
    public addAll(indexOrValues: number | Iterable<T>, values?: Iterable<T>): boolean {
        const index = typeof indexOrValues === "number" ? indexOrValues : this.size();
        const source = typeof indexOrValues === "number" ? values : indexOrValues;
        this.requirePositionIndex(index);
        if (source === undefined) throw new IllegalArgumentException("Collection must not be undefined");
        const additions = Array.from(source);
        if (additions.length === 0) return false;
        this.elements.splice(index, 0, ...additions);
        this.modificationCount++;
        return true;
    }

    /** Java remove(int). Use removeElement for an element when T may be number. */
    public remove(index: number): T {
        return this.removeAt(index);
    }
    public removeAt(index: number): T {
        this.requireElementIndex(index);
        const removed = this.elements.splice(index, 1)[0] as T;
        this.modificationCount++;
        return removed;
    }
    public removeElement(value: T): boolean {
        const index = this.indexOf(value);
        if (index < 0) return false;
        this.removeAt(index);
        return true;
    }
    public clear(): void {
        if (this.elements.length > 0) {
            this.elements.length = 0;
            this.modificationCount++;
        }
    }
    public indexOf(value: T): number {
        for (let i = 0; i < this.elements.length; i++) if (ArrayList.javaEquals(this.elements[i] as T, value)) return i;
        return -1;
    }
    public lastIndexOf(value: T): number {
        for (let i = this.elements.length - 1; i >= 0; i--)
            if (ArrayList.javaEquals(this.elements[i] as T, value)) return i;
        return -1;
    }
    public toArray(): T[] {
        return this.elements.slice();
    }

    public iterator(): Iterator<T> {
        const list = this;
        let cursor = 0;
        let lastReturned = -1;
        let expectedModificationCount = this.modificationCount;
        const check = (): void => {
            if (list.modificationCount !== expectedModificationCount)
                throw new IllegalStateException("Collection modified during iteration");
        };
        return {
            hasNext: (): boolean => {
                check();
                return cursor < list.size();
            },
            next: (): T => {
                check();
                if (cursor >= list.size()) throw new IndexOutOfBoundsException("No more elements");
                lastReturned = cursor;
                cursor++;
                return list.get(lastReturned);
            },
            remove: (): void => {
                check();
                if (lastReturned < 0) throw new IllegalStateException("next has not been called");
                list.removeAt(lastReturned);
                cursor = lastReturned;
                lastReturned = -1;
                expectedModificationCount = list.modificationCount;
            },
        };
    }

    public *[Symbol.iterator](): IterableIterator<T> {
        const iterator = this.iterator();
        while (iterator.hasNext()) yield iterator.next();
    }

    private requireElementIndex(index: number): void {
        if (!Number.isInteger(index) || index < 0 || index >= this.elements.length)
            throw new IndexOutOfBoundsException(`Index ${index}, size ${this.elements.length}`);
    }
    private requirePositionIndex(index: number): void {
        if (!Number.isInteger(index) || index < 0 || index > this.elements.length)
            throw new IndexOutOfBoundsException(`Index ${index}, size ${this.elements.length}`);
    }
    private static javaEquals(left: unknown, right: unknown): boolean {
        if (
            left === right ||
            (typeof left === "number" && typeof right === "number" && Number.isNaN(left) && Number.isNaN(right))
        )
            return true;
        if (
            left !== null &&
            typeof left === "object" &&
            "equals" in left &&
            typeof (left as { equals?: unknown }).equals === "function"
        )
            return (left as { equals(other: unknown): boolean }).equals(right);
        return false;
    }
}

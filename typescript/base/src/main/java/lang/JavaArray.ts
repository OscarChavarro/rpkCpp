import { ArrayIndexOutOfBoundsException } from "./ArrayIndexOutOfBoundsException.js";
import { NullPointerException } from "./NullPointerException.js";

/** Indexable storage of a Java array: a JavaScript array or a typed array. */
export interface JavaArrayLike<T> {
    readonly length: number;
    [index: number]: T;
}

/**
Java array access semantics for arrays stored in JavaScript arrays or typed
arrays, which ignore out of range writes and return `undefined` for out of
range reads: `a[i]` on a null array throws `NullPointerException`, and an
index outside `[0, a.length)` throws `ArrayIndexOutOfBoundsException` with
the HotSpot message.
*/
export class JavaArray {
    private constructor() {}

    /** Java `a[index]` (read). */
    public static get<T>(array: JavaArrayLike<T> | null, index: number): T {
        const a = JavaArray.check(array, index);
        return a[index]!;
    }

    /** Java `a[index] = value`. */
    public static set<T>(array: JavaArrayLike<T> | null, index: number, value: T): void {
        const a = JavaArray.check(array, index);
        a[index] = value;
    }

    /** Throws as a Java access `array[index]` would; returns the array. */
    public static check<T>(array: JavaArrayLike<T> | null, index: number): JavaArrayLike<T> {
        if (array === null) {
            throw new NullPointerException();
        }
        if (index < 0 || index >= array.length) {
            throw new ArrayIndexOutOfBoundsException("Index " + index + " out of bounds for length " + array.length);
        }
        return array;
    }
}

import { FundamentalEntity } from "../FundamentalEntity.js";

/** Growable Java-double-compatible array. */
export class ArrayListOfDoubles extends FundamentalEntity {
    private readonly increment: number;
    private assignedSize: number;
    private mysize = 0;
    private array: Float64Array;
    public constructor(increment: number) {
        super();
        if (!Number.isInteger(increment) || increment <= 0)
            throw new RangeError("increment must be a positive integer");
        this.increment = increment;
        this.assignedSize = increment;
        this.array = new Float64Array(increment);
    }
    public size(): number {
        return this.mysize;
    }
    public get(i: number): number {
        this.checkIndex(i);
        return this.array[i]!;
    }
    public set(i: number, value: number): void {
        this.checkIndex(i);
        this.array[i] = value;
    }
    public add(value: number): void {
        if (this.mysize >= this.assignedSize) this.grow();
        this.array[this.mysize++] = value;
    }
    public clean(): void {
        this.mysize = 0;
    }
    public sort(): void {
        if (this.mysize > 1) ArrayListOfDoubles.quicksort(this.array, 0, this.mysize - 1);
    }
    public static quicksort(array: Float64Array, left0: number, right0: number): void {
        let left = left0;
        let right = right0 + 1;
        const pivot = array[left0]!;
        do {
            do {
                left++;
            } while (left <= right0 && array[left]! < pivot);
            do {
                right--;
            } while (array[right]! > pivot);
            if (left < right) {
                const value = array[left]!;
                array[left] = array[right]!;
                array[right] = value;
            }
        } while (left <= right);
        const value = array[left0]!;
        array[left0] = array[right]!;
        array[right] = value;
        if (left0 < right) ArrayListOfDoubles.quicksort(array, left0, right);
        if (left < right0) ArrayListOfDoubles.quicksort(array, left, right0);
    }
    private grow(): void {
        const grown = new Float64Array(this.assignedSize + this.increment);
        grown.set(this.array.subarray(0, this.mysize));
        this.array = grown;
        this.assignedSize += this.increment;
    }
    private checkIndex(i: number): void {
        if (!Number.isInteger(i) || i < 0 || i >= this.array.length)
            throw new RangeError(`Array index out of bounds: ${i}`);
    }
}

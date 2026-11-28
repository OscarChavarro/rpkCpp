import { IllegalArgumentException } from "../lang/IllegalArgumentException.js";

/**
Stable merge sort with the observable comparison sequence of the Java
platform object sort (`java.util.TimSort` / `ComparableTimSort`, used by
`Arrays.sort(Object[])`, `List.sort` and `Collections.sort`).

ECMAScript engines also use TimSort variants, but with different run-length
and merge-collapse policies. The results only coincide for consistent
comparators; Vitral sorts some collections with deliberately non-symmetric
`compareTo` implementations (for example the splitter null edges), so the
Java policy is reproduced here: `MIN_MERGE = 32`, `MIN_GALLOP = 7`, the Java
minimum-run formula, the run-stack invariant and galloping merges.
*/
export class TimSort<T> {
    private static readonly MIN_MERGE = 32;
    private static readonly MIN_GALLOP = 7;

    private readonly a: T[];
    private readonly c: (left: T, right: T) => number;
    private minGallop = TimSort.MIN_GALLOP;
    private stackSize = 0;
    private readonly runBase: number[] = [];
    private readonly runLen: number[] = [];

    private constructor(a: T[], c: (left: T, right: T) => number) {
        this.a = a;
        this.c = c;
    }

    /** Sorts `a[lo, hi)` in place. */
    public static sort<T>(a: T[], lo: number, hi: number, c: (left: T, right: T) => number): void {
        let nRemaining = hi - lo;
        if (nRemaining < 2) {
            return; // Arrays of size 0 and 1 are always sorted
        }

        // If array is small, do a "mini-TimSort" with no merges
        if (nRemaining < TimSort.MIN_MERGE) {
            const initRunLen = TimSort.countRunAndMakeAscending(a, lo, hi, c);
            TimSort.binarySort(a, lo, hi, lo + initRunLen, c);
            return;
        }

        const ts = new TimSort<T>(a, c);
        const minRun = TimSort.minRunLength(nRemaining);
        do {
            // Identify next run
            let runLen = TimSort.countRunAndMakeAscending(a, lo, hi, c);

            // If run is short, extend to min(minRun, nRemaining)
            if (runLen < minRun) {
                const force = nRemaining <= minRun ? nRemaining : minRun;
                TimSort.binarySort(a, lo, lo + force, lo + runLen, c);
                runLen = force;
            }

            // Push run onto pending-run stack, and maybe merge
            ts.pushRun(lo, runLen);
            ts.mergeCollapse();

            // Advance to find next run
            lo += runLen;
            nRemaining -= runLen;
        } while (nRemaining !== 0);

        ts.mergeForceCollapse();
    }

    private static binarySort<T>(
        a: T[],
        lo: number,
        hi: number,
        start: number,
        c: (left: T, right: T) => number,
    ): void {
        if (start === lo) {
            start++;
        }
        for (; start < hi; start++) {
            const pivot = a[start]!;

            // Set left (and right) to the index where a[start] (pivot) belongs
            let left = lo;
            let right = start;
            while (left < right) {
                const mid = (left + right) >>> 1;
                if (c(pivot, a[mid]!) < 0) {
                    right = mid;
                } else {
                    left = mid + 1;
                }
            }

            for (let k = start; k > left; k--) {
                a[k] = a[k - 1]!;
            }
            a[left] = pivot;
        }
    }

    private static countRunAndMakeAscending<T>(
        a: T[],
        lo: number,
        hi: number,
        c: (left: T, right: T) => number,
    ): number {
        let runHi = lo + 1;
        if (runHi === hi) {
            return 1;
        }

        // Find end of run, and reverse range if descending
        if (c(a[runHi++]!, a[lo]!) < 0) {
            // Descending
            while (runHi < hi && c(a[runHi]!, a[runHi - 1]!) < 0) {
                runHi++;
            }
            TimSort.reverseRange(a, lo, runHi);
        } else {
            // Ascending
            while (runHi < hi && c(a[runHi]!, a[runHi - 1]!) >= 0) {
                runHi++;
            }
        }

        return runHi - lo;
    }

    private static reverseRange<T>(a: T[], lo: number, hi: number): void {
        hi--;
        while (lo < hi) {
            const t = a[lo]!;
            a[lo++] = a[hi]!;
            a[hi--] = t;
        }
    }

    private static minRunLength(n: number): number {
        let r = 0; // Becomes 1 if any 1 bits are shifted off
        while (n >= TimSort.MIN_MERGE) {
            r |= n & 1;
            n >>= 1;
        }
        return n + r;
    }

    private pushRun(runBase: number, runLen: number): void {
        this.runBase[this.stackSize] = runBase;
        this.runLen[this.stackSize] = runLen;
        this.stackSize++;
    }

    private mergeCollapse(): void {
        const runLen = this.runLen;
        while (this.stackSize > 1) {
            let n = this.stackSize - 2;
            if (
                (n > 0 && runLen[n - 1]! <= runLen[n]! + runLen[n + 1]!) ||
                (n > 1 && runLen[n - 2]! <= runLen[n]! + runLen[n - 1]!)
            ) {
                if (runLen[n - 1]! < runLen[n + 1]!) {
                    n--;
                }
            } else if (n < 0 || runLen[n]! > runLen[n + 1]!) {
                break; // Invariant is established
            }
            this.mergeAt(n);
        }
    }

    private mergeForceCollapse(): void {
        const runLen = this.runLen;
        while (this.stackSize > 1) {
            let n = this.stackSize - 2;
            if (n > 0 && runLen[n - 1]! < runLen[n + 1]!) {
                n--;
            }
            this.mergeAt(n);
        }
    }

    private mergeAt(i: number): void {
        const a = this.a;
        let base1 = this.runBase[i]!;
        let len1 = this.runLen[i]!;
        const base2 = this.runBase[i + 1]!;
        let len2 = this.runLen[i + 1]!;

        // Record the length of the combined runs; if i is the 3rd-last run
        // now, also slide over the last run (which isn't involved in this
        // merge). The current run (i+1) goes away in any case.
        this.runLen[i] = len1 + len2;
        if (i === this.stackSize - 3) {
            this.runBase[i + 1] = this.runBase[i + 2]!;
            this.runLen[i + 1] = this.runLen[i + 2]!;
        }
        this.stackSize--;

        // Find where the first element of run2 goes in run1. Prior elements
        // in run1 can be ignored (because they're already in place).
        const k = TimSort.gallopRight(a[base2]!, a, base1, len1, 0, this.c);
        base1 += k;
        len1 -= k;
        if (len1 === 0) {
            return;
        }

        // Find where the last element of run1 goes in run2. Subsequent
        // elements in run2 can be ignored (because they're already in place).
        len2 = TimSort.gallopLeft(a[base1 + len1 - 1]!, a, base2, len2, len2 - 1, this.c);
        if (len2 === 0) {
            return;
        }

        // Merge remaining runs, using tmp array with min(len1, len2) elements
        if (len1 <= len2) {
            this.mergeLo(base1, len1, base2, len2);
        } else {
            this.mergeHi(base1, len1, base2, len2);
        }
    }

    private static gallopLeft<T>(
        key: T,
        a: T[],
        base: number,
        len: number,
        hint: number,
        c: (left: T, right: T) => number,
    ): number {
        let lastOfs = 0;
        let ofs = 1;
        if (c(key, a[base + hint]!) > 0) {
            // Gallop right until a[base+hint+lastOfs] < key <= a[base+hint+ofs]
            const maxOfs = len - hint;
            while (ofs < maxOfs && c(key, a[base + hint + ofs]!) > 0) {
                lastOfs = ofs;
                ofs = (ofs << 1) + 1;
                if (ofs <= 0) {
                    // int overflow
                    ofs = maxOfs;
                }
            }
            if (ofs > maxOfs) {
                ofs = maxOfs;
            }

            // Make offsets relative to base
            lastOfs += hint;
            ofs += hint;
        } else {
            // key <= a[base + hint]
            // Gallop left until a[base+hint-ofs] < key <= a[base+hint-lastOfs]
            const maxOfs = hint + 1;
            while (ofs < maxOfs && c(key, a[base + hint - ofs]!) <= 0) {
                lastOfs = ofs;
                ofs = (ofs << 1) + 1;
                if (ofs <= 0) {
                    // int overflow
                    ofs = maxOfs;
                }
            }
            if (ofs > maxOfs) {
                ofs = maxOfs;
            }

            // Make offsets relative to base
            const tmp = lastOfs;
            lastOfs = hint - ofs;
            ofs = hint - tmp;
        }

        // Now a[base+lastOfs] < key <= a[base+ofs], so key belongs somewhere
        // to the right of lastOfs but no farther right than ofs. Do a binary
        // search, with invariant a[base + lastOfs - 1] < key <= a[base + ofs].
        lastOfs++;
        while (lastOfs < ofs) {
            const m = lastOfs + ((ofs - lastOfs) >>> 1);

            if (c(key, a[base + m]!) > 0) {
                lastOfs = m + 1; // a[base + m] < key
            } else {
                ofs = m; // key <= a[base + m]
            }
        }
        return ofs; // so a[base + ofs - 1] < key <= a[base + ofs]
    }

    private static gallopRight<T>(
        key: T,
        a: T[],
        base: number,
        len: number,
        hint: number,
        c: (left: T, right: T) => number,
    ): number {
        let ofs = 1;
        let lastOfs = 0;
        if (c(key, a[base + hint]!) < 0) {
            // Gallop left until a[b+hint - ofs] <= key < a[b+hint - lastOfs]
            const maxOfs = hint + 1;
            while (ofs < maxOfs && c(key, a[base + hint - ofs]!) < 0) {
                lastOfs = ofs;
                ofs = (ofs << 1) + 1;
                if (ofs <= 0) {
                    // int overflow
                    ofs = maxOfs;
                }
            }
            if (ofs > maxOfs) {
                ofs = maxOfs;
            }

            // Make offsets relative to b
            const tmp = lastOfs;
            lastOfs = hint - ofs;
            ofs = hint - tmp;
        } else {
            // a[b + hint] <= key
            // Gallop right until a[b+hint + lastOfs] <= key < a[b+hint + ofs]
            const maxOfs = len - hint;
            while (ofs < maxOfs && c(key, a[base + hint + ofs]!) >= 0) {
                lastOfs = ofs;
                ofs = (ofs << 1) + 1;
                if (ofs <= 0) {
                    // int overflow
                    ofs = maxOfs;
                }
            }
            if (ofs > maxOfs) {
                ofs = maxOfs;
            }

            // Make offsets relative to b
            lastOfs += hint;
            ofs += hint;
        }

        // Now a[b + lastOfs] <= key < a[b + ofs], so key belongs somewhere to
        // the right of lastOfs but no farther right than ofs. Do a binary
        // search, with invariant a[b + lastOfs - 1] <= key < a[b + ofs].
        lastOfs++;
        while (lastOfs < ofs) {
            const m = lastOfs + ((ofs - lastOfs) >>> 1);

            if (c(key, a[base + m]!) < 0) {
                ofs = m; // key < a[b + m]
            } else {
                lastOfs = m + 1; // a[b + m] <= key
            }
        }
        return ofs; // so a[b + ofs - 1] <= key < a[b + ofs]
    }

    private mergeLo(base1: number, len1: number, base2: number, len2: number): void {
        // Copy first run into temp array
        const a = this.a;
        const tmp = a.slice(base1, base1 + len1);

        let cursor1 = 0; // Indexes into tmp array
        let cursor2 = base2; // Indexes into a
        let dest = base1; // Indexes into a

        // Move first element of second run and deal with degenerate cases
        a[dest++] = a[cursor2++]!;
        if (--len2 === 0) {
            TimSort.arraycopy(tmp, cursor1, a, dest, len1);
            return;
        }
        if (len1 === 1) {
            TimSort.arraycopy(a, cursor2, a, dest, len2);
            a[dest + len2] = tmp[cursor1]!; // Last elt of run 1 to end of merge
            return;
        }

        const c = this.c;
        let minGallop = this.minGallop;
        outer: while (true) {
            let count1 = 0; // Number of times in a row that first run won
            let count2 = 0; // Number of times in a row that second run won

            /*
             * Do the straightforward thing until (if ever) one run starts
             * winning consistently.
             */
            do {
                if (c(a[cursor2]!, tmp[cursor1]!) < 0) {
                    a[dest++] = a[cursor2++]!;
                    count2++;
                    count1 = 0;
                    if (--len2 === 0) break outer;
                } else {
                    a[dest++] = tmp[cursor1++]!;
                    count1++;
                    count2 = 0;
                    if (--len1 === 1) break outer;
                }
            } while ((count1 | count2) < minGallop);

            /*
             * One run is winning so consistently that galloping may be a
             * huge win. So try that, and continue galloping until (if ever)
             * neither run appears to be winning consistently anymore.
             */
            do {
                count1 = TimSort.gallopRight(a[cursor2]!, tmp, cursor1, len1, 0, c);
                if (count1 !== 0) {
                    TimSort.arraycopy(tmp, cursor1, a, dest, count1);
                    dest += count1;
                    cursor1 += count1;
                    len1 -= count1;
                    if (len1 <= 1) break outer; // len1 == 1 || len1 == 0
                }
                a[dest++] = a[cursor2++]!;
                if (--len2 === 0) break outer;

                count2 = TimSort.gallopLeft(tmp[cursor1]!, a, cursor2, len2, 0, c);
                if (count2 !== 0) {
                    TimSort.arraycopy(a, cursor2, a, dest, count2);
                    dest += count2;
                    cursor2 += count2;
                    len2 -= count2;
                    if (len2 === 0) break outer;
                }
                a[dest++] = tmp[cursor1++]!;
                if (--len1 === 1) break outer;
                minGallop--;
            } while ((count1 >= TimSort.MIN_GALLOP ? 1 : 0) | (count2 >= TimSort.MIN_GALLOP ? 1 : 0));
            if (minGallop < 0) minGallop = 0;
            minGallop += 2; // Penalize for leaving gallop mode
        } // End of "outer" loop
        this.minGallop = minGallop < 1 ? 1 : minGallop; // Write back to field

        if (len1 === 1) {
            TimSort.arraycopy(a, cursor2, a, dest, len2);
            a[dest + len2] = tmp[cursor1]!; //  Last elt of run 1 to end of merge
        } else if (len1 === 0) {
            throw new IllegalArgumentException("Comparison method violates its general contract!");
        } else {
            TimSort.arraycopy(tmp, cursor1, a, dest, len1);
        }
    }

    private mergeHi(base1: number, len1: number, base2: number, len2: number): void {
        // Copy second run into temp array
        const a = this.a;
        const tmp = a.slice(base2, base2 + len2);
        const tmpBase = 0;

        let cursor1 = base1 + len1 - 1; // Indexes into a
        let cursor2 = tmpBase + len2 - 1; // Indexes into tmp array
        let dest = base2 + len2 - 1; // Indexes into a

        // Move last element of first run and deal with degenerate cases
        a[dest--] = a[cursor1--]!;
        if (--len1 === 0) {
            TimSort.arraycopy(tmp, tmpBase, a, dest - (len2 - 1), len2);
            return;
        }
        if (len2 === 1) {
            dest -= len1;
            cursor1 -= len1;
            TimSort.arraycopy(a, cursor1 + 1, a, dest + 1, len1);
            a[dest] = tmp[cursor2]!;
            return;
        }

        const c = this.c;
        let minGallop = this.minGallop;
        outer: while (true) {
            let count1 = 0; // Number of times in a row that first run won
            let count2 = 0; // Number of times in a row that second run won

            /*
             * Do the straightforward thing until (if ever) one run
             * appears to win consistently.
             */
            do {
                if (c(tmp[cursor2]!, a[cursor1]!) < 0) {
                    a[dest--] = a[cursor1--]!;
                    count1++;
                    count2 = 0;
                    if (--len1 === 0) break outer;
                } else {
                    a[dest--] = tmp[cursor2--]!;
                    count2++;
                    count1 = 0;
                    if (--len2 === 1) break outer;
                }
            } while ((count1 | count2) < minGallop);

            /*
             * One run is winning so consistently that galloping may be a
             * huge win. So try that, and continue galloping until (if ever)
             * neither run appears to be winning consistently anymore.
             */
            do {
                count1 = len1 - TimSort.gallopRight(tmp[cursor2]!, a, base1, len1, len1 - 1, c);
                if (count1 !== 0) {
                    dest -= count1;
                    cursor1 -= count1;
                    len1 -= count1;
                    TimSort.arraycopy(a, cursor1 + 1, a, dest + 1, count1);
                    if (len1 === 0) break outer;
                }
                a[dest--] = tmp[cursor2--]!;
                if (--len2 === 1) break outer;

                count2 = len2 - TimSort.gallopLeft(a[cursor1]!, tmp, tmpBase, len2, len2 - 1, c);
                if (count2 !== 0) {
                    dest -= count2;
                    cursor2 -= count2;
                    len2 -= count2;
                    TimSort.arraycopy(tmp, cursor2 + 1, a, dest + 1, count2);
                    if (len2 <= 1) break outer; // len2 == 1 || len2 == 0
                }
                a[dest--] = a[cursor1--]!;
                if (--len1 === 0) break outer;
                minGallop--;
            } while ((count1 >= TimSort.MIN_GALLOP ? 1 : 0) | (count2 >= TimSort.MIN_GALLOP ? 1 : 0));
            if (minGallop < 0) minGallop = 0;
            minGallop += 2; // Penalize for leaving gallop mode
        } // End of "outer" loop
        this.minGallop = minGallop < 1 ? 1 : minGallop; // Write back to field

        if (len2 === 1) {
            dest -= len1;
            cursor1 -= len1;
            TimSort.arraycopy(a, cursor1 + 1, a, dest + 1, len1);
            a[dest] = tmp[cursor2]!; // Move first elt of run2 to front of merge
        } else if (len2 === 0) {
            throw new IllegalArgumentException("Comparison method violates its general contract!");
        } else {
            TimSort.arraycopy(tmp, tmpBase, a, dest - (len2 - 1), len2);
        }
    }

    /** `System.arraycopy`, including the overlapping-range semantics. */
    private static arraycopy<T>(src: T[], srcPos: number, dest: T[], destPos: number, length: number): void {
        if (src === dest && srcPos < destPos) {
            for (let i = length - 1; i >= 0; i--) {
                dest[destPos + i] = src[srcPos + i]!;
            }
            return;
        }
        for (let i = 0; i < length; i++) {
            dest[destPos + i] = src[srcPos + i]!;
        }
    }
}

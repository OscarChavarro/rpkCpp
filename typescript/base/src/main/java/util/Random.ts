export class Random {
    private static readonly MULTIPLIER = 0x5deece66dn;
    private static readonly ADDEND = 0xbn;
    private static readonly MASK = (1n << 48n) - 1n;

    private seed: bigint;
    private nextNextGaussian = 0.0;
    private haveNextNextGaussian = false;

    public constructor(seed: number = Date.now()) {
        this.seed = 0n;
        this.setSeed(seed);
    }

    public setSeed(seed: number): void {
        this.seed = (BigInt(globalThis.Math.trunc(seed)) ^ Random.MULTIPLIER) & Random.MASK;
        this.haveNextNextGaussian = false;
    }

    protected next(bits: number): number {
        this.seed = (this.seed * Random.MULTIPLIER + Random.ADDEND) & Random.MASK;
        return Number(this.seed >> BigInt(48 - bits));
    }

    /**
    `Random.nextInt(int bound)`: Java's algorithm, so the same seed gives the
    same sequence; the `int` overflow test of the rejection loop is done with
    32-bit wrapping arithmetic.
    @param bound upper bound (exclusive), positive
    @return a value in [0, bound)
    */
    public nextInt(bound: number): number {
        if (bound <= 0) {
            throw new RangeError("bound must be positive");
        }
        if ((bound & -bound) === bound) {
            return Number((BigInt(bound) * BigInt(this.next(31))) >> 31n);
        }
        let bits: number;
        let val: number;
        do {
            bits = this.next(31);
            val = bits % bound;
        } while (((bits - val + (bound - 1)) | 0) < 0);
        return val;
    }

    public nextDouble(): number {
        const high = BigInt(this.next(26));
        const low = BigInt(this.next(27));
        const combined = (high << 27n) + low;
        return Number(combined) / Number(1n << 53n);
    }

    /** `Random.nextBoolean()`. */
    public nextBoolean(): boolean {
        return this.next(1) !== 0;
    }

    /**
    `Random.nextGaussian()`: Java's polar method, so the same seed gives the
    same sequence. Java uses `StrictMath.log`/`StrictMath.sqrt`; V8's
    `Math.log` is the same fdlibm algorithm and `Math.sqrt` is exact.
    */
    public nextGaussian(): number {
        if (this.haveNextNextGaussian) {
            this.haveNextNextGaussian = false;
            return this.nextNextGaussian;
        }
        let v1: number;
        let v2: number;
        let s: number;
        do {
            v1 = 2 * this.nextDouble() - 1;
            v2 = 2 * this.nextDouble() - 1;
            s = v1 * v1 + v2 * v2;
        } while (s >= 1 || s === 0);
        const multiplier = globalThis.Math.sqrt((-2 * globalThis.Math.log(s)) / s);
        this.nextNextGaussian = v2 * multiplier;
        this.haveNextNextGaussian = true;
        return v1 * multiplier;
    }
}

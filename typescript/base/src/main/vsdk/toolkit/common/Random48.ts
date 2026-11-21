/**
Central 48-bit RNG used by the stochastic raytracing code, replicating the
POSIX rand48 family (drand48/srand48/seed48) exactly as glibc implements it,
so that this port draws the very same random sequence as the C++ port and
produces bit-identical golden test images.

Note that glibc leaves the 48-bit state zero-initialized until srand48 or
seed48 is called (it does not apply the POSIX default seed 0x1234ABCD330E),
and the C++ port renders without seeding, so the state starts at zero here
as well.
*/
export class Random48 {
    // Same constants glibc uses for drand48/srand48/seed48
    private static readonly MULTIPLIER = 0x5deece66dn;
    private static readonly ADDEND = 0xbn;
    private static readonly MASK_48 = (1n << 48n) - 1n;
    private static readonly TWO_POW_48 = 281474976710656.0; // 2^48

    private static state = 0n;

    private constructor() {}

    public static srand48(seed: number): void {
        const high = BigInt(globalThis.Math.trunc(seed)) & 0xffffffffn;
        Random48.state = ((high << 16n) | 0x330en) & Random48.MASK_48;
    }

    /**
    erand48: the drand48 step on a caller-owned state xsubi[3] (unsigned
    16-bit words, low word first), which is advanced in place
    */
    public static erand48(xsubi: number[]): number {
        let x =
            (BigInt((xsubi[2] ?? 0) & 0xffff) << 32n) |
            (BigInt((xsubi[1] ?? 0) & 0xffff) << 16n) |
            BigInt((xsubi[0] ?? 0) & 0xffff);
        x = (Random48.MULTIPLIER * x + Random48.ADDEND) & Random48.MASK_48;
        xsubi[0] = Number(x & 0xffffn);
        xsubi[1] = Number((x >> 16n) & 0xffffn);
        xsubi[2] = Number((x >> 32n) & 0xffffn);
        return Number(x) / Random48.TWO_POW_48;
    }

    public static drand48(): number {
        Random48.state = (Random48.MULTIPLIER * Random48.state + Random48.ADDEND) & Random48.MASK_48;
        return Number(Random48.state) / Random48.TWO_POW_48;
    }

    /**
  Sets the 48-bit generator state from newSeed[3] (each entry treated as an
  unsigned 16-bit value, low word first) and returns the previous state,
  mirroring the C library's seed48() semantics used by SeedConfig.
  */
    public static seed48(newSeed: number[]): number[] {
        const previous = [
            Number(Random48.state & 0xffffn),
            Number((Random48.state >> 16n) & 0xffffn),
            Number((Random48.state >> 32n) & 0xffffn),
        ];
        Random48.state =
            ((BigInt((newSeed[2] ?? 0) & 0xffff) << 32n) |
                (BigInt((newSeed[1] ?? 0) & 0xffff) << 16n) |
                BigInt((newSeed[0] ?? 0) & 0xffff)) &
            Random48.MASK_48;
        return previous;
    }
}

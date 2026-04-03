package vsdk.toolkit.numericalAnalysis.quasiMonteCarlo;
/**
31bit 4D base-2 Niederreiter quasiMonteCarlo series, ACM TOMS Algorithm 738, dec 94.

See also:

P. Bratley, B. Fox, H. Niederreiter, "Implementation and Tests of
Low-Discrepancy Sequences", ACM Trans Mod Comp Sim Vol 2 Nr 3,
July  1992 p195

Philippe.Bekaert@cs.kuleuven.ac.be, Thu Oct 23 1997
*/


/**
Niederreiter quasiMonteCarlo sample series (dimension 4, base 2, 31 bits, skip 4096)
*/
public class Niederreiter31 {
    // Number of samples to be skipped from the beginning of the series in order to
    // deal with the "initial zeroes" phenomenon
    public static final int SKIP = 4096;

    // Dimension of the samples generated
    public static final int DIMEN = 4;

    // Number of bits in an integer, excluding the sign bit
    public static final int NBITS = 31;

    // 1/2^NBITS
    public static final double RECIP = 1.0 / 2147483648.0;

    // 2^NBITS
    public static final double RECIP1 = 2147483648.0;

    // 2^NBITS
    public static final long NBITS_POW = (1L << NBITS);

    // 2^(NBITS-1)
    public static final long NBITS_POW1 = (1L << (NBITS - 1));

    private static final long[][] directionNumbers = {
        {
            0x40000000L, 0x20000000L, 0x10000000L, 0x08000000L,
            0x04000000L, 0x02000000L, 0x01000000L, 0x00800000L,
            0x00400000L, 0x00200000L, 0x00100000L, 0x00080000L,
            0x00040000L, 0x00020000L, 0x00010000L, 0x00008000L,
            0x00004000L, 0x00002000L, 0x00001000L, 0x00000800L,
            0x00000400L, 0x00000200L, 0x00000100L, 0x00000080L,
            0x00000040L, 0x00000020L, 0x00000010L, 0x00000008L,
            0x00000004L, 0x00000002L, 0x00000001L
        },
        {
            0x40000000L, 0x60000000L, 0x50000000L, 0x78000000L,
            0x44000000L, 0x66000000L, 0x55000000L, 0x7f800000L,
            0x40400000L, 0x60600000L, 0x50500000L, 0x78780000L,
            0x44440000L, 0x66660000L, 0x55550000L, 0x7fff8000L,
            0x40004000L, 0x60006000L, 0x50005000L, 0x78007800L,
            0x44004400L, 0x66006600L, 0x55005500L, 0x7f807f80L,
            0x40404040L, 0x60606060L, 0x50505050L, 0x78787878L,
            0x44444444L, 0x66666666L, 0x55555555L
        },
        {
            0x60000000L, 0x48000000L, 0x38000000L, 0x7a000000L,
            0x5e000000L, 0x36800000L, 0x65800000L, 0x4b200000L,
            0x3e600000L, 0x7ec80000L, 0x5db80000L, 0x315a0000L,
            0x603e0000L, 0x487e8000L, 0x385d8000L, 0x7a312000L,
            0x5e606000L, 0x36c84800L, 0x65b83800L, 0x4b5a7a00L,
            0x3e3e5e00L, 0x7efeb680L, 0x5ddde580L, 0x31116b20L,
            0x60005e60L, 0x480036c8L, 0x380065b8L, 0x7a004b5aL,
            0x5e003e3eL, 0x36807efeL, 0x65805dddL
        },
        {
            0x70000000L, 0x62000000L, 0x46000000L, 0x1e000000L,
            0x2c400000L, 0x5ac00000L, 0x37c00000L, 0x7d880000L,
            0x6b580000L, 0x44b80000L, 0x1b310000L, 0x26230000L,
            0x5c470000L, 0x38c62000L, 0x71c46000L, 0x6389e000L,
            0x471ac400L, 0x1e7dac00L, 0x2cf27c00L, 0x5bacd880L,
            0x3719b580L, 0x7c7a6b80L, 0x6af4d310L, 0x45a88230L,
            0x1b580070L, 0x26b80062L, 0x5d310046L, 0x3823001eL,
            0x7007002cL, 0x6206205aL, 0x46046037L
        }
    };

    private static NiederreiterCore core = new NiederreiterCore(
        directionNumbers,
        SKIP,
        NBITS_POW,
        NBITS_POW1,
        DIMEN,
        NBITS);

    /**
    31bit 4D base-2 Niederreiter quasiMonteCarlo series, ACM TOMS Algorithm 738, dec 94.

    See also:

    P. Bratley, B. Fox, H. Niederreiter, "Implementation and Tests of
    Low-Discrepancy Sequences", ACM Trans Mod Comp Sim Vol 2 Nr 3,
    July  1992 p195

    Philippe.Bekaert@cs.kuleuven.ac.be, Thu Oct 23 1997
    */
    public static long[] niederreiter31(long index) {
        return core.sample(index);
    }

    public static long[] NextNiedInRange31(
        long[] idx,
        int dir,
        int nmsb,
        long msb1,
        long rmsb2) {
        return core.nextInRange(idx, dir, nmsb, msb1, rmsb2);
    }

    public static long radicalInverse31(long n) {
        return core.radicalInverse(n);
    }

    public static void foldSample31(long[] xi1, long[] xi2) {
        core.foldSample(xi1, xi2);
    }
}

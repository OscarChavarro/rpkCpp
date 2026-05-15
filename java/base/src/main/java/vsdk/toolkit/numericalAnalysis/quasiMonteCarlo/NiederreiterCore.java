package vsdk.toolkit.numericalAnalysis.quasiMonteCarlo;

/**
Shared Niederreiter core implementation for 31-bit and 63-bit variants.
*/
class NiederreiterCore {
    private final long[][] cj;
    private final long skip;
    private final long nBitsPow;
    private final long nBitsPow1;
    private final int dimension;
    private final int numberOfBits;
    private final long[] nied;
    private long count;

    public NiederreiterCore(
        long[][] cj,
        long skip,
        long nBitsPow,
        long nBitsPow1,
        int dimension,
        int numberOfBits) {
        this.cj = cj;
        this.skip = skip;
        this.nBitsPow = nBitsPow;
        this.nBitsPow1 = nBitsPow1;
        this.dimension = dimension;
        this.numberOfBits = numberOfBits;
        this.nied = new long[dimension];
        this.count = 0;
    }

    public long[] sample(long n) {
        long diff;

        n += skip;
        diff = n ^ count; // Contains 1s where bits in n and count differ
        int bitIndex = 0;
        while (diff != 0) {
            if ((diff & 1L) != 0) {
                nied[0] ^= cj[0][bitIndex];
                nied[1] ^= cj[1][bitIndex];
                nied[2] ^= cj[2][bitIndex];
                nied[3] ^= cj[3][bitIndex];
            }

            bitIndex++;
            diff >>>= 1;
        }

        count = n;
        return nied;
    }

    public long[] nextInRange(
        long[] idx,
        int dir,
        int nmsb,
        long msb1,
        long rmsb2) {
        long mask;
        long rmask;
        long diff;
        long c;
        long i;
        long step;
        int bitIndex;

        step = 1L << nmsb;
        mask = step - 1;
        rmask = mask << (numberOfBits - nmsb);
        msb1 &= mask;
        rmsb2 &= rmask;

        i = idx[0] + skip;
        if (dir >= 0) {
            i = (((Long.compareUnsigned(i & mask, msb1) <= 0) ? i : i + mask) & ~mask) | msb1;
        }
        else {
            i = (((Long.compareUnsigned(i & mask, msb1) >= 0) ? i : i - mask) & ~mask) | msb1;
            // Keep unsigned modular arithmetic semantics from the original implementation.
            step = -step;
        }

        c = count;
        diff = (i ^ c) & mask;
        bitIndex = 0;
        while (diff != 0) {
            if ((diff & 1L) != 0) {
                nied[1] ^= cj[1][bitIndex];
            }
            diff >>>= 1;
            bitIndex++;
        }

        do {
            diff = (i ^ c) >>> nmsb;
            bitIndex = nmsb;
            while (diff != 0) {
                if ((diff & 1L) != 0) {
                    nied[1] ^= cj[1][bitIndex];
                }
                diff >>>= 1;
                bitIndex++;
            }
            c = i;
            i += step;
            if (Long.compareUnsigned(i, nBitsPow) >= 0) {
                System.err.printf(
                    "\nOverflow in Niederreiter sequence. A %d-bit sequence is not enough???\n",
                    numberOfBits);
                return null;
            }
        }
        while ((nied[1] & rmask) != rmsb2);

        diff = c ^ count;
        bitIndex = 0;
        while (diff != 0) {
            if ((diff & 1L) != 0) {
                nied[0] ^= cj[0][bitIndex];
                nied[2] ^= cj[2][bitIndex];
                nied[3] ^= cj[3][bitIndex];
            }
            diff >>>= 1;
            bitIndex++;
        }
        count = c;

        idx[0] = count - skip;
        return nied;
    }

    public long radicalInverse(long n) {
        long inv = 0;
        long f = nBitsPow1;
        while (n != 0) {
            if ((n & 1L) != 0) {
                inv |= f;
            }
            f >>>= 1;
            n >>>= 1;
            if ((n & 1L) != 0) {
                inv |= f;
            }
            f >>>= 1;
            n >>>= 1;
            if ((n & 1L) != 0) {
                inv |= f;
            }
            f >>>= 1;
            n >>>= 1;
            if ((n & 1L) != 0) {
                inv |= f;
            }
            f >>>= 1;
            n >>>= 1;
        }
        return inv;
    }

    public void foldSample(long[] xi1, long[] xi2) {
        long u = xi1[0];
        long v = xi2[0];
        long d;
        long m;

        u = (u & ~3L) | 1L; // Clear last two bits / displace point
        v = (v & ~3L) | 1L; // So it will not lay on a cell boundary
        d = (u & v) & ~1L; // Contains 1's where folding is needed

        m = nBitsPow; // Marks most significant bits
        while (d != 0) {
            if ((d & nBitsPow1) != 0) {
                // Fold
                u = (u & m) | (~(u - 1) & ~m);
                v = (v & m) | (~(v - 1) & ~m);
            }
            m |= m >>> 1;
            d <<= 1;
        }

        xi1[0] = u;
        xi2[0] = v;
    }
}

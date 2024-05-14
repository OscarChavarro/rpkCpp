/**
4D base-2 Niederreiter quasiMonteCarlo series,
[CHOW1994] ACM TOMS Algorithm 738, dec 94

See also:

[BRAT1992] P. Bratley, B. Fox, H. Niederreiter, "Implementation and Tests of
Low-Discrepancy Sequences", ACM Trans Mod Comp Sim Vol 2 Nr 3,
July  1992 p195

Philippe.Bekaert@cs.kuleuven.ac.be, Thu Oct 23 1997
*/

#include <cstdlib>
#include <cstdio>

static NiederreiterIndex nied[DIMEN] = {0, 0, 0, 0};
static NiederreiterIndex count = 0;

/**
Computes the base-2 31bits 4D Niederreiter sample with index n
*/
NiederreiterIndex *
Nied(NiederreiterIndex n) {
    NiederreiterIndex diff;
    NiederreiterIndex *cj0 = cj[0];
    NiederreiterIndex *cj1 = cj[1];
    NiederreiterIndex *cj2 = cj[2];
    NiederreiterIndex *cj3 = cj[3];

    n += SKIP;
    diff = n ^ count; // Contains 1s where bits in n and count differ
    while ( diff ) {
        if ( diff & 1 ) {
            nied[0] ^= *cj0;
            nied[1] ^= *cj1;
            nied[2] ^= *cj2;
            nied[3] ^= *cj3;
        }

        cj0++;
        cj1++;
        cj2++;
        cj3++;
        diff >>= 1;
    }

    count = n;
    return nied;
}

/**
Finds the next Niederreiter sample following or preceeding the
sample with index *idx in a range defined by nmsb, msb1, rmsb2
*/
NiederreiterIndex *
NextNiedInRange(
    NiederreiterIndex *idx,
    int dir,
    int nmsb,
    NiederreiterIndex msb1,
    NiederreiterIndex rmsb2)
{
    NiederreiterIndex mask;
    NiederreiterIndex rmask;
    NiederreiterIndex diff;
    NiederreiterIndex c;
    NiederreiterIndex i;
    NiederreiterIndex step;
    NiederreiterIndex *cj0;
    NiederreiterIndex *cj1;
    NiederreiterIndex *cj2;
    NiederreiterIndex *cj3;

    step = 1 << nmsb;
    mask = step - 1;
    rmask = mask << (NBITS - nmsb);
    msb1 &= mask;
    rmsb2 &= rmask;

    i = *idx + SKIP;
    if ( dir >= 0 ) {
        i = (((i & mask) <= msb1 ? i : i + mask) & ~mask) | msb1;
    } else {
        i = (((i & mask) >= msb1 ? i : i - mask) & ~mask) | msb1;
        step = -step;
    }

    c = count;
    diff = (i ^ c) & mask;
    cj1 = cj[1];
    while ( diff ) {
        if ( diff & 1 ) {
            nied[1] ^= *cj1;
        }
        diff >>= 1;
        cj1++;
    }

    do {
        diff = (i ^ c) >> nmsb;
        cj1 = cj[1] + nmsb;
        while ( diff ) {
            if ( diff & 1 ) {
                nied[1] ^= *cj1;
            }
            diff >>= 1;
            cj1++;
        }
        c = i;
        i += step;
        if ( i >= NBITS_POW) {
            fprintf(stderr, "\nOverflow in Niederreiter sequence. A %d-bit sequence is not enough???\n", NBITS);
            abort();
        }
    } while ( (nied[1] & rmask) != rmsb2 );

    cj0 = cj[0];
    cj2 = cj[2];
    cj3 = cj[3];
    diff = c ^ count;
    while ( diff ) {
        if ( diff & 1 ) {
            nied[0] ^= *cj0;
            nied[2] ^= *cj2;
            nied[3] ^= *cj3;
        }
        diff >>= 1;
        cj0++;
        cj2++;
        cj3++;
    }
    count = c;

    *idx = count - SKIP;
    return nied;
}

/**
Computes the base-2 radical inverse of the given number (31 bits)
*/
NiederreiterIndex
radicalInverse(NiederreiterIndex n) {
    NiederreiterIndex inv = 0;
    NiederreiterIndex f = NBITS_POW1;
    while ( n ) {
        if ( n & 1 ) {
            inv |= f;
        }
        f >>= 1;
        n >>= 1;
        if ( n & 1 ) {
            inv |= f;
        }
        f >>= 1;
        n >>= 1;
        if ( n & 1 ) {
            inv |= f;
        }
        f >>= 1;
        n >>= 1;
        if ( n & 1 ) {
            inv |= f;
        }
        f >>= 1;
        n >>= 1;
    }
    return inv;
}

/**
"folds" a sample in the unit square to the standard triangle (0, 0), (1, 0), (0, 1)
*/
void
foldSample(NiederreiterIndex *xi1, NiederreiterIndex *xi2) {
    NiederreiterIndex u = *xi1;
    NiederreiterIndex v = *xi2;
    NiederreiterIndex d;
    NiederreiterIndex m;

    u = (u & ~3) | 1; // Clear last two bits / displace point
    v = (v & ~3) | 1; // So it will not lay on a cell boundary
    d = (u & v) & ~1; // Contains 1's where folding is needed

    m = NBITS_POW; // Marks most significant bits
    while ( d ) {
        if ( d & NBITS_POW1 ) {
            // Fold
            u = (u & m) | (~(u - 1) & ~m);
            v = (v & m) | (~(v - 1) & ~m);
        }
        m |= m >> 1;
        d <<= 1;
    }

    *xi1 = u;
    *xi2 = v;
}

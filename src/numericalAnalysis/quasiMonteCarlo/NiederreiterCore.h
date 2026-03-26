/**
Shared Niederreiter core implementation for 31-bit and 63-bit variants.
*/

#ifndef __NIEDERREITER_CORE__
#define __NIEDERREITER_CORE__

#include <cstdlib>
#include "java/lang/System.h"

template<typename IndexType, unsigned Dimension, unsigned NumberOfBits>
class NiederreiterCore {
  public:
    NiederreiterCore(
        const IndexType (&cj)[Dimension][NumberOfBits],
        IndexType skip,
        IndexType nBitsPow,
        IndexType nBitsPow1)
        : cj_(cj),
          skip_(skip),
          nBitsPow_(nBitsPow),
          nBitsPow1_(nBitsPow1),
          nied_{},
          count_(0)
    {
        static_assert(Dimension >= 4, "NiederreiterCore expects at least 4 dimensions");
        static_assert(NumberOfBits > 0, "NiederreiterCore expects a positive bit width");
    }

    IndexType *
    sample(IndexType n) {
        IndexType diff;
        const IndexType *cj0 = cj_[0];
        const IndexType *cj1 = cj_[1];
        const IndexType *cj2 = cj_[2];
        const IndexType *cj3 = cj_[3];

        n += skip_;
        diff = n ^ count_; // Contains 1s where bits in n and count differ
        while ( diff ) {
            if ( diff & 1 ) {
                nied_[0] ^= *cj0;
                nied_[1] ^= *cj1;
                nied_[2] ^= *cj2;
                nied_[3] ^= *cj3;
            }

            cj0++;
            cj1++;
            cj2++;
            cj3++;
            diff >>= 1;
        }

        count_ = n;
        return nied_;
    }

    IndexType *
    nextInRange(
        IndexType *idx,
        int dir,
        int nmsb,
        IndexType msb1,
        IndexType rmsb2)
    {
        IndexType mask;
        IndexType rmask;
        IndexType diff;
        IndexType c;
        IndexType i;
        IndexType step;
        const IndexType *cj0;
        const IndexType *cj1;
        const IndexType *cj2;
        const IndexType *cj3;

        step = static_cast<IndexType>(1) << nmsb;
        mask = step - 1;
        rmask = mask << (NumberOfBits - nmsb);
        msb1 &= mask;
        rmsb2 &= rmask;

        i = *idx + skip_;
        if ( dir >= 0 ) {
            i = (((i & mask) <= msb1 ? i : i + mask) & ~mask) | msb1;
        } else {
            i = (((i & mask) >= msb1 ? i : i - mask) & ~mask) | msb1;
            // Keep unsigned modular arithmetic semantics from the original implementation.
            step = static_cast<IndexType>(0) - step;
        }

        c = count_;
        diff = (i ^ c) & mask;
        cj1 = cj_[1];
        while ( diff ) {
            if ( diff & 1 ) {
                nied_[1] ^= *cj1;
            }
            diff >>= 1;
            cj1++;
        }

        do {
            diff = (i ^ c) >> nmsb;
            cj1 = cj_[1] + nmsb;
            while ( diff ) {
                if ( diff & 1 ) {
                    nied_[1] ^= *cj1;
                }
                diff >>= 1;
                cj1++;
            }
            c = i;
            i += step;
            if ( i >= nBitsPow_ ) {
                java::lang::System::err.printf(
                    "\nOverflow in Niederreiter sequence. A %u-bit sequence is not enough???\n",
                    NumberOfBits);
                abort();
            }
        } while ( (nied_[1] & rmask) != rmsb2 );

        cj0 = cj_[0];
        cj2 = cj_[2];
        cj3 = cj_[3];
        diff = c ^ count_;
        while ( diff ) {
            if ( diff & 1 ) {
                nied_[0] ^= *cj0;
                nied_[2] ^= *cj2;
                nied_[3] ^= *cj3;
            }
            diff >>= 1;
            cj0++;
            cj2++;
            cj3++;
        }
        count_ = c;

        *idx = count_ - skip_;
        return nied_;
    }

    IndexType
    radicalInverse(IndexType n) const {
        IndexType inv = 0;
        IndexType f = nBitsPow1_;
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

    void
    foldSample(IndexType *xi1, IndexType *xi2) const {
        IndexType u = *xi1;
        IndexType v = *xi2;
        IndexType d;
        IndexType m;

        u = (u & ~3) | 1; // Clear last two bits / displace point
        v = (v & ~3) | 1; // So it will not lay on a cell boundary
        d = (u & v) & ~1; // Contains 1's where folding is needed

        m = nBitsPow_; // Marks most significant bits
        while ( d ) {
            if ( d & nBitsPow1_ ) {
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

  private:
    const IndexType (&cj_)[Dimension][NumberOfBits];
    const IndexType skip_;
    const IndexType nBitsPow_;
    const IndexType nBitsPow1_;
    IndexType nied_[Dimension];
    IndexType count_;
};

#endif

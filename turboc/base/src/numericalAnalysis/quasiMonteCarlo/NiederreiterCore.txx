/**
Shared Niederreiter core implementation for 31-bit and 63-bit variants.
*/

#ifndef __NIEDERREITER_CORE__
#define __NIEDERREITER_CORE__

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
          nied_(),
          count_(0)
    {
        typedef char niederreiter_core_requires_dimension_at_least_4[(Dimension >= 4) ? 1 : -1];
        typedef char niederreiter_core_requires_positive_bit_count[(NumberOfBits > 0) ? 1 : -1];
        (void)sizeof(niederreiter_core_requires_dimension_at_least_4);
        (void)sizeof(niederreiter_core_requires_positive_bit_count);
    }

    IndexType *
    sample(IndexType n) {
        IndexType diff;

        n += skip_;
        diff = n ^ count_; // Contains 1s where bits in n and count differ
        unsigned bitIndex = 0;
        while ( diff ) {
            if ( diff & 1 ) {
                nied_[0] ^= cj_[0][bitIndex];
                nied_[1] ^= cj_[1][bitIndex];
                nied_[2] ^= cj_[2][bitIndex];
                nied_[3] ^= cj_[3][bitIndex];
            }

            bitIndex++;
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
        unsigned bitIndex;

        step = ((IndexType)(1)) << nmsb;
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
            step = ((IndexType)(0)) - step;
        }

        c = count_;
        diff = (i ^ c) & mask;
        bitIndex = 0;
        while ( diff ) {
            if ( diff & 1 ) {
                nied_[1] ^= cj_[1][bitIndex];
            }
            diff >>= 1;
            bitIndex++;
        }

        do {
            diff = (i ^ c) >> nmsb;
            bitIndex = ((unsigned)(nmsb));
            while ( diff ) {
                if ( diff & 1 ) {
                    nied_[1] ^= cj_[1][bitIndex];
                }
                diff >>= 1;
                bitIndex++;
            }
            c = i;
            i += step;
            if ( i >= nBitsPow_ ) {
                System::err.printf(
                    "\nOverflow in Niederreiter sequence. A %u-bit sequence is not enough???\n",
                    NumberOfBits);
                return NULL;
            }
        } while ( (nied_[1] & rmask) != rmsb2 );

        diff = c ^ count_;
        bitIndex = 0;
        while ( diff ) {
            if ( diff & 1 ) {
                nied_[0] ^= cj_[0][bitIndex];
                nied_[2] ^= cj_[2][bitIndex];
                nied_[3] ^= cj_[3][bitIndex];
            }
            diff >>= 1;
            bitIndex++;
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

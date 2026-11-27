#include <float.h>
#include <math.h>
#include <string.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "java/lang/Double.h"
#include "java/lang/Math.h"

// 2^53, the first integer magnitude that binary64 cannot hold with every
// neighbor
static const double TWO_TO_53 = 9007199254740992.0;

/**
Number of trailing zero bits of the 53-bit significand of a normal value,
read from its binary64 representation (little-endian, as Math::nextAfter
reads it).
*/
static int
trailingZeroBits(double value)
{
    unsigned char bytes[8];
    memcpy(bytes, &value, sizeof(bytes));
    int count = 0;
    int i;
    for ( i = 0; i < 7; i++ ) {
        unsigned char b = bytes[i];
        if ( i == 6 ) {
            b = (unsigned char)(b & 0x0F);
        }
        if ( b == 0 ) {
            count += i == 6 ? 4 : 8;
            continue;
        }
        while ( (b & 1) == 0 ) {
            b = (unsigned char)(b >> 1);
            count++;
        }
        return count;
    }
    return 52;
}

/**
Splits a finite nonzero value into an odd integer significand and the binary
exponent of its lowest bit: value = significand * 2^lowBit (frexp, ldexp and
fmod are exact). Also gives the exponent of its highest bit.
*/
static void
oddSignificand(double value, double& significand, int& lowBit, int& highBit)
{
    int exponent = 0;
    double mantissa = frexp(fabs(value), &exponent);
    if ( fabs(value) >= DBL_MIN ) {
        const int zeros = trailingZeroBits(value);
        significand = ldexp(mantissa, 53 - zeros);
        lowBit = exponent - 53 + zeros;
        highBit = exponent - 1;
        return;
    }
    // Subnormal values: rare, by halving
    double integer = ldexp(mantissa, 53);
    int low = exponent - 53;
    while ( fmod(integer, 2.0) == 0.0 ) {
        integer = integer / 2.0;
        low++;
    }
    significand = integer;
    lowBit = low;
    highBit = exponent - 1;
}

FloatingInterval::FloatingInterval() : lower(0), upper(0), valid(FALSE) {}
FloatingInterval::FloatingInterval(double lo, double hi, BOOLEAN ok) : lower(lo), upper(hi), valid(ok) {}

BOOLEAN FloatingInterval::isSupportedEnvironment()
{
    if (FLT_RADIX!=2 || DBL_MANT_DIG!=53 || DBL_MAX_EXP!=1024) return FALSE;
    // Round to nearest (ties to even), observed through stored results
    volatile double one=1;
    volatile double quarter=ldexp(1.0,-54);
    volatile double threeQuarters=ldexp(3.0,-54);
    volatile double sum=one+quarter;
    if (sum!=1) return FALSE;
    sum=one+threeQuarters;
    if (sum!=1+ldexp(1.0,-52)) return FALSE;
    sum=-one-quarter;
    if (sum!=-1) return FALSE;
    sum=-one-threeQuarters;
    if (sum!=-1-ldexp(1.0,-52)) return FALSE;
    volatile double tiny=Double::MIN_VALUE;
    volatile double product=tiny*one;
    return product!=0 && Math::nextAfter(0,1)==Double::MIN_VALUE;
}

FloatingInterval FloatingInterval::exact(double value)
{
    if (Double::isFinite(value)) return FloatingInterval(value,value,TRUE);
    return FloatingInterval();
}

FloatingInterval FloatingInterval::spanning(double lo, double hi)
{
    if (Double::isFinite(lo) && Double::isFinite(hi) && lo<=hi)
        return FloatingInterval(lo,hi,TRUE);
    return FloatingInterval();
}

FloatingInterval FloatingInterval::around(double center, double halfWidth)
{
    if (!Double::isFinite(center) || !Double::isFinite(halfWidth) || halfWidth<0)
        return FloatingInterval();
    if (halfWidth==0) return exact(center);
    const FloatingInterval low=exact(center).subtract(exact(halfWidth));
    const FloatingInterval high=exact(center).add(exact(halfWidth));
    if (low.valid && high.valid) return spanning(low.lower,high.upper);
    return FloatingInterval();
}

FloatingInterval FloatingInterval::rounded(double lo, double hi)
{
    if (!Double::isFinite(lo) || !Double::isFinite(hi) || lo>hi) return FloatingInterval();
    lo=Math::nextAfter(lo,Double::NEGATIVE_INFINITY);
    hi=Math::nextAfter(hi,Double::POSITIVE_INFINITY);
    if (!Double::isFinite(lo) || !Double::isFinite(hi)) return FloatingInterval();
    return FloatingInterval(lo,hi,TRUE);
}

BOOLEAN FloatingInterval::exactSum(double a, double b, double& sum)
{
    if (b==0) { sum=a; return TRUE; }
    if (a==0) { sum=b; return TRUE; }
    double ma, mb;
    int lowA, highA, lowB, highB;
    oddSignificand(a,ma,lowA,highA);
    oddSignificand(b,mb,lowB,highB);
    const int low=lowA<lowB ? lowA : lowB;
    const int high=highA>highB ? highA : highB;
    // Both operands in one window of 53 bits: their integer images are exact
    if (high-low>52) return FALSE;
    const double x=ldexp(a,-low);
    const double y=ldexp(b,-low);
    volatile double integerSum=x+y;
    // Rounding is monotonic: a computed magnitude below 2^53 means the exact
    // integer sum is below 2^53, hence representable and computed exactly
    if (!(fabs(integerSum)<TWO_TO_53)) return FALSE;
    volatile double result=ldexp(integerSum,low);
    if (!Double::isFinite(result)) return FALSE;
    sum=result;
    return TRUE;
}

BOOLEAN FloatingInterval::exactProduct(double a, double b, double& product)
{
    if (a==0 || b==0) { product=a*b; return TRUE; }
    double ma, mb;
    int lowA, highA, lowB, highB;
    oddSignificand(a,ma,lowA,highA);
    oddSignificand(b,mb,lowB,highB);
    volatile double significand=ma*mb;
    // As the sum: an odd integer product below 2^53 is computed exactly
    if (!(significand<TWO_TO_53)) return FALSE;
    // The cpp version does not trust its residual below 2^-969; this version
    // does not prove more than it
    volatile double result=a*b;
    if (!Double::isFinite(result) || Math::abs(result)<DBL_MIN*TWO_TO_53) return FALSE;
    if (result!=ldexp((a<0)!=(b<0) ? -significand : significand,lowA+lowB)) return FALSE;
    product=result;
    return TRUE;
}

FloatingInterval FloatingInterval::add(const FloatingInterval& b) const
{
    if (!valid || !b.valid) return FloatingInterval();
    if (lower==upper && b.lower==b.upper) {
        double sum=0;
        if (exactSum(lower,b.lower,sum)) return FloatingInterval(sum,sum,TRUE);
    }
    volatile double lo=lower+b.lower;
    volatile double hi=upper+b.upper;
    return rounded(lo,hi);
}
FloatingInterval FloatingInterval::subtract(const FloatingInterval& b) const
{
    if (!valid || !b.valid) return FloatingInterval();
    return add(FloatingInterval(-b.upper,-b.lower,TRUE));
}
FloatingInterval FloatingInterval::multiply(const FloatingInterval& b) const
{
    if (!valid || !b.valid) return FloatingInterval();
    if (lower==upper && b.lower==b.upper) {
        double product=0;
        if (exactProduct(lower,b.lower,product)) return FloatingInterval(product,product,TRUE);
    }
    volatile double values[4];
    values[0]=lower*b.lower; values[1]=lower*b.upper; values[2]=upper*b.lower; values[3]=upper*b.upper;
    double lo=values[0],hi=values[0];
    int i;
    for (i=1;i<4;++i) { if (values[i]<lo) lo=values[i]; if (values[i]>hi) hi=values[i]; }
    for (i=0;i<4;++i) if (!Double::isFinite(values[i])) return FloatingInterval();
    return rounded(lo,hi);
}
FloatingInterval FloatingInterval::divide(const FloatingInterval& b) const
{
    if (!valid || !b.valid || (b.lower<=0 && b.upper>=0)) return FloatingInterval();
    volatile double values[4];
    values[0]=lower/b.lower; values[1]=lower/b.upper; values[2]=upper/b.lower; values[3]=upper/b.upper;
    double lo=values[0],hi=values[0];
    int i;
    for (i=1;i<4;++i) { if (values[i]<lo) lo=values[i]; if (values[i]>hi) hi=values[i]; }
    for (i=0;i<4;++i) if (!Double::isFinite(values[i])) return FloatingInterval();
    return rounded(lo,hi);
}
FloatingInterval FloatingInterval::square() const
{
    if (!valid) return FloatingInterval();
    volatile double a=lower*lower;
    volatile double b=upper*upper;
    if (!Double::isFinite(a) || !Double::isFinite(b)) return FloatingInterval();
    const double hi=a>b?a:b;
    const double lo=(lower<=0 && upper>=0)?0:(a<b?a:b);
    FloatingInterval result=rounded(lo,hi);
    if (result.valid && result.lower<0) result.lower=0;
    return result;
}
FloatingInterval FloatingInterval::squareRoot() const
{
    // Intersect with the square-root domain. Callers must know that the
    // represented exact value is nonnegative (e.g. a sum of squares).
    if (!valid || upper<0) return FloatingInterval();
    volatile double lo=Math::sqrt(lower<0?0.0:lower);
    volatile double hi=Math::sqrt(upper);
    FloatingInterval result=rounded(lo,hi);
    if (result.valid && result.lower<0) result.lower=0;
    return result;
}

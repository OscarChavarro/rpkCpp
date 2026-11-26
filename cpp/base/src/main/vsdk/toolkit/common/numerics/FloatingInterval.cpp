#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include <fenv.h>
#include <cmath>
#include <cfloat>
// The kernel runtime (vitralStdC++) has no <limits>: there the compiler's
// predefined __GCC_IEC_559 tells the same
#if defined(__has_include)
#if __has_include(<limits>)
#include <limits>
#define VITRAL_FLOATING_INTERVAL_NUMERIC_LIMITS 1
#endif
#endif

FloatingInterval::FloatingInterval() : lower(0), upper(0), valid(false) {}
FloatingInterval::FloatingInterval(double lo, double hi, bool ok) : lower(lo), upper(hi), valid(ok) {}

bool FloatingInterval::isSupportedEnvironment()
{
#if (!defined(__GNUC__) && !defined(__clang__)) || defined(__FAST_MATH__)
    return false;
#endif
    // __STDC_IEC_559__ comes from the C library (glibc defines it, macOS does not),
    // so ask the C++ implementation instead.
#ifdef VITRAL_FLOATING_INTERVAL_NUMERIC_LIMITS
    if (!std::numeric_limits<double>::is_iec559) return false;
#elif !defined(__GCC_IEC_559) || __GCC_IEC_559 <= 0
    return false;
#endif
    if (FLT_RADIX!=2 || DBL_MANT_DIG!=53 || DBL_MAX_EXP!=1024 || FLT_EVAL_METHOD!=0 ||
        fegetround()!=FE_TONEAREST) return false;
    volatile double tiny=java::Double::MIN_VALUE;
    volatile double one=1;
    return tiny*one!=0 && java::Math::nextAfter(0,1)==java::Double::MIN_VALUE;
}

FloatingInterval FloatingInterval::exact(double value)
{
    return java::Double::isFinite(value) ? FloatingInterval(value,value,true) : FloatingInterval();
}

FloatingInterval FloatingInterval::spanning(double lo, double hi)
{
    return java::Double::isFinite(lo) && java::Double::isFinite(hi) && lo<=hi ?
        FloatingInterval(lo,hi,true) : FloatingInterval();
}

FloatingInterval FloatingInterval::around(double center, double halfWidth)
{
    if (!java::Double::isFinite(center) || !java::Double::isFinite(halfWidth) || halfWidth<0)
        return FloatingInterval();
    if (halfWidth==0) return exact(center);
    const FloatingInterval low=exact(center).subtract(exact(halfWidth));
    const FloatingInterval high=exact(center).add(exact(halfWidth));
    return low.valid && high.valid ? spanning(low.lower,high.upper) : FloatingInterval();
}

FloatingInterval FloatingInterval::rounded(double lo, double hi)
{
    if (!java::Double::isFinite(lo) || !java::Double::isFinite(hi) || lo>hi) return FloatingInterval();
    lo=java::Math::nextAfter(lo,java::Double::NEGATIVE_INFINITY);
    hi=java::Math::nextAfter(hi,java::Double::POSITIVE_INFINITY);
    if (!java::Double::isFinite(lo) || !java::Double::isFinite(hi)) return FloatingInterval();
    return FloatingInterval(lo,hi,true);
}

FloatingInterval FloatingInterval::add(const FloatingInterval& b) const
{
    if (!valid || !b.valid) return FloatingInterval();
    if (lower==upper && b.lower==b.upper) {
        // TwoSum: a zero rounding error means the point sum is exact.
        const double sum=lower+b.lower;
        const double shifted=sum-lower;
        const double error=(lower-(sum-shifted))+(b.lower-shifted);
        if (java::Double::isFinite(sum) && error==0) return FloatingInterval(sum,sum,true);
    }
    return rounded(lower+b.lower,upper+b.upper);
}
FloatingInterval FloatingInterval::subtract(const FloatingInterval& b) const
{
    if (!valid || !b.valid) return FloatingInterval();
    return add(FloatingInterval(-b.upper,-b.lower,true));
}
FloatingInterval FloatingInterval::multiply(const FloatingInterval& b) const
{
    if (!valid || !b.valid) return FloatingInterval();
    if (lower==upper && b.lower==b.upper) {
        // A correctly rounded fused residual of zero means the product is exact.
        // Below 2^-969 the residual could underflow to zero, so do not trust it.
        const double product=lower*b.lower;
        if (java::Double::isFinite(product) && (product==0 ? (lower==0 || b.lower==0) :
            java::Math::abs(product)>=DBL_MIN*9007199254740992.0) && std::fma(lower,b.lower,-product)==0)
            return FloatingInterval(product,product,true);
    }
    const double values[]={lower*b.lower,lower*b.upper,upper*b.lower,upper*b.upper};
    double lo=values[0],hi=values[0];
    for (int i=1;i<4;++i) { if (values[i]<lo) lo=values[i]; if (values[i]>hi) hi=values[i]; }
    for (int i=0;i<4;++i) if (!java::Double::isFinite(values[i])) return FloatingInterval();
    return rounded(lo,hi);
}
FloatingInterval FloatingInterval::divide(const FloatingInterval& b) const
{
    if (!valid || !b.valid || (b.lower<=0 && b.upper>=0)) return FloatingInterval();
    const double values[]={lower/b.lower,lower/b.upper,upper/b.lower,upper/b.upper};
    double lo=values[0],hi=values[0];
    for (int i=1;i<4;++i) { if (values[i]<lo) lo=values[i]; if (values[i]>hi) hi=values[i]; }
    for (int i=0;i<4;++i) if (!java::Double::isFinite(values[i])) return FloatingInterval();
    return rounded(lo,hi);
}
FloatingInterval FloatingInterval::square() const
{
    if (!valid) return FloatingInterval();
    const double a=lower*lower,b=upper*upper;
    if (!java::Double::isFinite(a) || !java::Double::isFinite(b)) return FloatingInterval();
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
    FloatingInterval result=rounded(java::Math::sqrt(lower<0?0:lower),java::Math::sqrt(upper));
    if (result.valid && result.lower<0) result.lower=0;
    return result;
}

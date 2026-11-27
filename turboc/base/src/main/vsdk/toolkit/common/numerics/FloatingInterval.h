#ifndef __FLOATING_INTERVAL__
#define __FLOATING_INTERVAL__

#include "vsdk/toolkit/common/VSDK.h"

// Closed binary64 enclosure with one-ULP outward rounding per operation;
// sums and products of point intervals that are proven exact stay points.
// Invalid inputs, overflow and unsupported floating-point environments produce
// an invalid interval. The implementation requires IEEE-754 binary64,
// round-to-nearest, gradual underflow and correctly rounded elementary sqrt.
//
// C++ port note (turboc, internals differ from cpp): the cpp version proves
// exact point sums with TwoSum and exact point products with a fused
// multiply-add residual, and supports only FLT_EVAL_METHOD 0 (strict
// binary64). Turbo C++ 3.0 and gcc 2.7.2 evaluate on the x87 in extended
// precision, where TwoSum is not reliable, and have neither fma nor
// <fenv.h>. Here:
// - the outward rounding of one ULP still encloses the exact result of an
//   operation rounded twice (to extended, then to binary64), so x87
//   evaluation is supported;
// - exactness is proven from the integer significands of the operands
//   (frexp / ldexp, which are exact), never from a rounded result: a point
//   product is exact when the product of the odd significands is below 2^53,
//   a point sum when both operands fit in a common 53 bit window and their
//   integer sum stays below 2^53. Sums that cancel from wider windows are not
//   proven exact (they become one-ULP intervals, a looser but valid
//   enclosure), so this version proves the same or less than cpp;
// - the environment check tests round-to-nearest, binary64 and gradual
//   underflow by computing, without <fenv.h>.
class FloatingInterval {
private:
    double lower;
    double upper;
    BOOLEAN valid;
    FloatingInterval(double lowerValue, double upperValue, BOOLEAN isValid);
    static FloatingInterval rounded(double low, double high);
    static BOOLEAN exactSum(double a, double b, double& sum);
    static BOOLEAN exactProduct(double a, double b, double& product);
public:
    FloatingInterval();
    static BOOLEAN isSupportedEnvironment();
    static FloatingInterval exact(double value);
    // The closed interval [lowerValue, upperValue]; invalid unless finite and ordered.
    static FloatingInterval spanning(double lowerValue, double upperValue);
    // An enclosure of [center - halfWidth, center + halfWidth]; exact when the
    // half width is zero, invalid for non-finite values or a negative width.
    static FloatingInterval around(double center, double halfWidth);
    BOOLEAN isValid() const { return valid; }
    double getLower() const { return lower; }
    double getUpper() const { return upper; }
    FloatingInterval add(const FloatingInterval& other) const;
    FloatingInterval subtract(const FloatingInterval& other) const;
    FloatingInterval multiply(const FloatingInterval& other) const;
    FloatingInterval divide(const FloatingInterval& other) const;
    FloatingInterval square() const;
    FloatingInterval squareRoot() const;
};

#endif

#ifndef __FLOATING_INTERVAL__
#define __FLOATING_INTERVAL__

// Closed binary64 enclosure with one-ULP outward rounding per operation;
// sums and products of point intervals that are proven exact stay points.
// Invalid inputs, overflow and unsupported floating-point environments produce
// an invalid interval. The implementation requires IEEE-754 binary64,
// round-to-nearest, gradual underflow and correctly rounded elementary sqrt.
class FloatingInterval {
private:
    double lower;
    double upper;
    bool valid;
    FloatingInterval(double lowerValue, double upperValue, bool isValid);
    static FloatingInterval rounded(double low, double high);
public:
    FloatingInterval();
    static bool isSupportedEnvironment();
    static FloatingInterval exact(double value);
    // The closed interval [lowerValue, upperValue]; invalid unless finite and ordered.
    static FloatingInterval spanning(double lowerValue, double upperValue);
    // An enclosure of [center - halfWidth, center + halfWidth]; exact when the
    // half width is zero, invalid for non-finite values or a negative width.
    static FloatingInterval around(double center, double halfWidth);
    bool isValid() const { return valid; }
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

#ifndef __VITRAL_JAVA_LANG_MATH__
#define __VITRAL_JAVA_LANG_MATH__

#include <cmath>
#include <stdlib.h>
namespace java {
class Math {
  public:
    static constexpr double E = 2.7182818284590452354;
    static constexpr double PI = 3.14159265358979323846;

    static double floor(double a);
    static float floor(float a);
    static double ceil(double a);
    static long round(double a);
    static double log(double a);
    static float log(float a);
    static double log10(double a);
    static double sin(double a);
    static float sin(float a);
    static double cos(double a);
    static float cos(float a);
    static double tan(double a);
    static float tan(float a);
    static double acos(double a);
    static float acos(float a);
    static double asin(double a);
    static float asin(float a);
    static double atan(double a);
    static double exp(double a);
    static float exp(float a);
    static double pow(double a, double e);
    static float pow(float a, float e);
    static double abs(double a);
    static float abs(float a);
    static int abs(int a);
    static long abs(long a);
    static int min(int a, int b);
    static int max(int a, int b);
    static float min(float a, float b);
    static float max(float a, float b);
    static double min(double a, double b);
    static double max(double a, double b);
    static long min(long a, long b);
    static long max(long a, long b);
    static float sqrt(float a);
    static double sqrt(double a);
    static double nextAfter(double start, double direction);
    static int getExponent(double a);
    static int getExponent(float a);
    static double toRadians(double angdeg);
    static double toDegrees(double angrad);
    static double scalb(double a, int scaleFactor);
    static float scalb(float a, int scaleFactor);
};

// As Java 9+ (the reference JDK): a single multiplication by the rounded
// conversion constant, not `angdeg / 180.0 * PI` (Java 8)
inline double
Math::toRadians(double angdeg) {
    return angdeg * 0.017453292519943295;
}

inline double
Math::toDegrees(double angrad) {
    // As Java 9+: multiplication by the rounded conversion constant
    return angrad * 57.29577951308232;
}

inline double
Math::ceil(double a) {
    return std::ceil(a);
}

inline float
Math::floor(float a) {
    return std::floor(a);
}

inline double
Math::floor(double a) {
    return std::floor(a);
}

inline long
Math::round(double a) {
    return std::lround(a);
}

inline float
Math::log(float a) {
    return std::log(a);
}

inline double
Math::log(double a) {
    return std::log(a);
}

inline double
Math::log10(double a) {
    return std::log10(a);
}

inline float
Math::sin(float a) {
    return std::sin(a);
}

inline double
Math::sin(double a) {
    return std::sin(a);
}

inline float
Math::cos(float a) {
    return std::cos(a);
}

inline double
Math::cos(double a) {
    return std::cos(a);
}

inline float
Math::tan(float a) {
    return std::tan(a);
}

inline double
Math::tan(double a) {
    return std::tan(a);
}

inline float
Math::acos(float a) {
    return std::acos(a);
}

inline double
Math::acos(double a) {
    return std::acos(a);
}

inline float
Math::asin(float a) {
    return std::asin(a);
}

inline double
Math::asin(double a) {
    return std::asin(a);
}

inline double
Math::atan(double a) {
    return std::atan(a);
}

inline float
Math::exp(float a) {
    return std::exp(a);
}

inline double
Math::exp(double a) {
    return std::exp(a);
}

inline float
Math::pow(float a, float e) {
    return std::pow(a, e);
}

inline double
Math::pow(double a, double e) {
    return std::pow(a, e);
}

inline double
Math::abs(double a) {
    // Same as java.lang.Math.abs: -0.0 yields +0.0 and NaN stays NaN
    return (a <= 0.0) ? 0.0 - a : a;
}

inline float
Math::abs(float a) {
    // Same as java.lang.Math.abs: -0.0f yields +0.0f and NaN stays NaN
    return (a <= 0.0f) ? 0.0f - a : a;
}

inline int
Math::abs(int a) {
    return ::abs(a);
}

inline long
Math::abs(long a) {
    return ::labs(a);
}

inline int
Math::min(int a, int b) {
    return a < b ? a : b;
}

inline int
Math::max(int a, int b) {
    return a > b ? a : b;
}

inline float
Math::min(float a, float b) {
    return a < b ? a : b;
}

inline float
Math::max(float a, float b) {
    return a > b ? a : b;
}

inline double
Math::min(double a, double b) {
    return a < b ? a : b;
}

inline double
Math::max(double a, double b) {
    return a > b ? a : b;
}

inline long
Math::min(long a, long b) {
    return a < b ? a : b;
}

inline long
Math::max(long a, long b) {
    return a > b ? a : b;
}

inline float
Math::sqrt(float a) {
    return std::sqrt(a);
}

inline double
Math::sqrt(double a) {
    return std::sqrt(a);
}

inline double
Math::nextAfter(double start, double direction) {
    return std::nextafter(start, direction);
}

inline int
Math::getExponent(double a) {
    return std::ilogb(a);
}

inline int
Math::getExponent(float a) {
    return std::ilogb(a);
}

inline double
Math::scalb(double a, int scaleFactor) {
    return std::ldexp(a, scaleFactor);
}

inline float
Math::scalb(float a, int scaleFactor) {
    return std::ldexp(a, scaleFactor);
}

}
#endif

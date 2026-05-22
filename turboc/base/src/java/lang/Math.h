#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __JAVA_MATH__
#define __JAVA_MATH__

#include "common/VSDK.h"

#include <math.h>

class Math {
  public:
    #define E 2.7182818284590452354
    #define PI 3.14159265358979323846

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
    static double atan(double a);
    static double exp(double a);
    static float exp(float a);
    static double pow(double a, double e);
    static float pow(float a, float e);
    static double abs(double a);
    static float abs(float a);
    static int min(int a, int b);
    static int max(int a, int b);
    static float min(float a, float b);
    static float max(float a, float b);
    static float sqrt(float a);
    static double sqrt(double a);
    static int getExponent(double a);
    static int getExponent(float a);
    static double scalb(double a, int scaleFactor);
    static float scalb(float a, int scaleFactor);
};

inline double
Math::ceil(double a) {
    return ::ceil(a);
}

inline float
Math::floor(float a) {
    return ::floor(a);
}

inline double
Math::floor(double a) {
    return ::floor(a);
}

inline long
Math::round(double a) {
    return (long)(a >= 0.0 ? ::floor(a + 0.5) : ::ceil(a - 0.5));
}

inline float
Math::log(float a) {
    return ::log(a);
}

inline double
Math::log(double a) {
    return ::log(a);
}

inline double
Math::log10(double a) {
    return ::log10(a);
}

inline float
Math::sin(float a) {
    return ::sin(a);
}

inline double
Math::sin(double a) {
    return ::sin(a);
}

inline float
Math::cos(float a) {
    return ::cos(a);
}

inline double
Math::cos(double a) {
    return ::cos(a);
}

inline float
Math::tan(float a) {
    return ::tan(a);
}

inline double
Math::tan(double a) {
    return ::tan(a);
}

inline float
Math::acos(float a) {
    return ::acos(a);
}

inline double
Math::acos(double a) {
    return ::acos(a);
}

inline double
Math::atan(double a) {
    return ::atan(a);
}

inline float
Math::exp(float a) {
    return ::exp(a);
}

inline double
Math::exp(double a) {
    return ::exp(a);
}

inline float
Math::pow(float a, float e) {
    return ::pow(a, e);
}

inline double
Math::pow(double a, double e) {
    return ::pow(a, e);
}

inline double
Math::abs(double a) {
    return a < 0.0 ? -a : a;
}

inline float
Math::abs(float a) {
    return a < 0.0 ? -a : a;
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

inline float
Math::sqrt(float a) {
    return ::sqrt(a);
}

inline double
Math::sqrt(double a) {
    return ::sqrt(a);
}

inline int
Math::getExponent(double a) {
    int exponent;
    float fa = (float)a;

    if ( fa == 0.0f ) {
        return 0;
    }
    ::frexp(fa, &exponent);
    return exponent - 1;
}

inline int
Math::getExponent(float a) {
    int exponent;

    if ( a == 0.0f ) {
        return 0;
    }
    ::frexp(a, &exponent);
    return exponent - 1;
}

inline double
Math::scalb(double a, int scaleFactor) {
    return ::ldexp(a, scaleFactor);
}

inline float
Math::scalb(float a, int scaleFactor) {
    return ::ldexp(a, scaleFactor);
}


#endif

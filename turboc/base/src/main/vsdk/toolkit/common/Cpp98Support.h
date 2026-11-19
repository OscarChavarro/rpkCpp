#ifndef __CPP98_SUPPORT__
#define __CPP98_SUPPORT__

#include <stdarg.h>
#include <string.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"

// Portable va_list copy helper.
#ifndef va_copy
    #ifdef __va_copy
        #define va_copy(destination, source) __va_copy(destination, source)
    #else
        #define va_copy(destination, source) \
            memcpy(&(destination), &(source), sizeof(va_list))
    #endif
#endif

/**
Portable numeric conversion and math helpers for legacy compilers.
*/
class Cpp98Support {
  public:
    // Returns a String.
    static String toString(int value);
    static String toString(unsigned int value);
    static String toString(long value);
    static String toString(unsigned long value);
#ifdef VSDK_HAS_LONG_LONG
    static String toString(long long value);
    static String toString(unsigned long long value);
#endif
    static String toString(float value);
    static String toString(double value);
    static String toString(long double value);

    // When one argument is NaN, the other is returned.
    static double fmax(double a, double b);
    static double fmin(double a, double b);

    // Halfway cases are rounded away from zero.
    static double round(double value);
    static long lround(double value);
    static long long llround(double value);

    // Euclidean norm and integer binary exponent.
    static double hypot(double x, double y);
    static int ilogb(double value);

    // Quiet NaN, as C99 NAN.
    static double nan();

    // Floating-point classification.
    static BOOLEAN isNaN(double value);
    static BOOLEAN isInfinite(double value);
    static BOOLEAN isFinite(double value);
    static BOOLEAN signBit(double value);

    // Absolute value without floating-point truncation.
    static int abs(int value);
    static long abs(long value);
#ifdef VSDK_HAS_LONG_LONG
    static long long abs(long long value);
#endif
    static float abs(float value);
    static double abs(double value);

  private:
    Cpp98Support();
};

#endif

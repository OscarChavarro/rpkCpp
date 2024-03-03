#ifndef __FLOAT__
#define __FLOAT__

#include <cmath>

#define EPSILON 1e-6

// Yes, float or double makes a difference!
#define REAL double

/**
Tests whether two floating point numbers are equal within the given tolerance
*/
inline bool
doubleEqual(double a, double b, double tolerance) {
    return (a - b) > -tolerance && (a - b) < tolerance;
}

inline unsigned long
unsignedLongMax(unsigned long a, unsigned long b) {
    return a > b ? a : b;
}

inline float
floatMax(float a, float b) {
    return a > b ? a : b;
}

inline int
intMax(int a, int b) {
    return a > b ? a : b;
}

inline float
floatMin(float a, float b) {
    return a < b ? a : b;
}

inline int
intMin(int a, int b) {
    return a < b ? a : b;
}

inline char
charMin(char a, char b) {
    return a < b ? a : b;
}

inline REAL
realAbs(REAL A) {
    return A < 0.0 ? -A : A;
}

// Common to vector types
#define X_NORMAL 0
#define Y_NORMAL 1
#define Z_NORMAL 2
#define X_GREATER 0x01
#define Y_GREATER 0x02
#define Z_GREATER 0x04
#define XYZ_EQUAL 0x08

#endif

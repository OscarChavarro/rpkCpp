#ifndef __FLOAT__
#define __FLOAT__

#define EPSILON 1e-6
#define EPSILON_FLOAT 1e-6f

/**
Tests whether two floating point numbers are equal within the given tolerance
*/
inline bool
doubleEqual(double a, double b, double tolerance) {
    return (a - b) > -tolerance && (a - b) < tolerance;
}

#endif

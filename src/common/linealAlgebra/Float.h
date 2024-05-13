#ifndef __FLOAT__
#define __FLOAT__

extern const double EPSILON;
extern const float EPSILON_FLOAT;

/**
Tests whether two floating point numbers are equal within the given tolerance
*/
inline bool
doubleEqual(double a, double b, double tolerance) {
    return (a - b) > -tolerance && (a - b) < tolerance;
}

#endif

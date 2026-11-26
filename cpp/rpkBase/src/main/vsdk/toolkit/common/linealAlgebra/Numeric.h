#ifndef __NUMERIC__
#define __NUMERIC__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Numeric {
  public:
    static const double HUGE_DOUBLE_VALUE;
    static const double EPSILON;

    static bool doubleEqual(double a, double b, double tolerance);
    static bool vectorsEqual(const Vector3Dd &a, const Vector3Dd &b, double tolerance);
    static bool floatCompare(double x, double y);
    static void roundDeltaToZero(double &x, double epsilon);
};

/**
Tests whether two floating point numbers are equal within the given tolerance
*/
inline bool
Numeric::doubleEqual(double a, double b, double tolerance) {
    return (a - b) > -tolerance && (a - b) < tolerance;
}

/**
Two vectors are equal if their components are equal within the given
tolerance, compared in double precision as doubleEqual does
*/
inline bool
Numeric::vectorsEqual(const Vector3Dd &a, const Vector3Dd &b, const double tolerance) {
    return doubleEqual(a.x(), b.x(), tolerance) &&
        doubleEqual(a.y(), b.y(), tolerance) &&
        doubleEqual(a.z(), b.z(), tolerance);
}

inline void
Numeric::roundDeltaToZero(double &x, double epsilon) {
    if ( x <= epsilon && x >= -epsilon ) {
        x = 0;
    }
}

#endif

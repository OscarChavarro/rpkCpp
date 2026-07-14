#ifndef NUMERIC__
#define NUMERIC__

class Numeric {
  public:
    static const double HUGE_DOUBLE_VALUE;
    static const float HUGE_FLOAT_VALUE;
    static const double EPSILON;
    static const float EPSILON_FLOAT;

    static bool doubleEqual(double a, double b, double tolerance);
    static bool floatCompare(float x, float y);
    static void roundDeltaToZero(double &x, double epsilon);
};

/**
Tests whether two floating point numbers are equal within the given tolerance
*/
inline bool
Numeric::doubleEqual(double a, double b, double tolerance) {
    return (a - b) > -tolerance && (a - b) < tolerance;
}

inline void
Numeric::roundDeltaToZero(double &x, double epsilon) {
    if ( x <= epsilon && x >= -epsilon ) {
        x = 0;
    }
}

#endif

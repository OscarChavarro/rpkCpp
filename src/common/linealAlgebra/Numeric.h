#ifndef __NUMERIC__
#define __NUMERIC__

typedef int (*QSORT_CALLBACK_TYPE)(const void *, const void *);

class Numeric {
  public:
    static const double HUGE_DOUBLE_VALUE;
    static const float HUGE_FLOAT_VALUE;
    static const double EPSILON;
    static const float EPSILON_FLOAT;

    /**
    Tests whether two floating point numbers are equal within the given tolerance
    */
    static inline bool
    doubleEqual(double a, double b, double tolerance) {
        return (a - b) > -tolerance && (a - b) < tolerance;
    }

    static int floatCompare(const float *x, const float *y);
};

#endif

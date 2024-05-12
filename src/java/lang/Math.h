#ifndef __MATH__
#define __MATH__

#include <cmath>

namespace java {
class Math {
public:
    static double abs(double a);
    static int min(int a, int b);
    static int max(int a, int b);
    static float min(float a, float b);
    static float max(float a, float b);
    static float sqrt(float a);
    static double sqrt(double a);
};

inline double
Math::abs(double a) {
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
    return std::sqrt(a);
}

inline double
Math::sqrt(double a) {
    return std::sqrt(a);
}
}
#endif

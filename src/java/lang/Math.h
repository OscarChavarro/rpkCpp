#ifndef __JAVA_MATH__
#define __JAVA_MATH__

#include <cmath>

namespace java {
class Math {
public:
    static double sin(double a);
    static float sin(float a);
    static double cos(double a);
    static float cos(float a);
    static double tan(double a);
    static float tan(float a);
    static double asin(double a);
    static float asin(float a);
    static double acos(double a);
    static float acos(float a);
    static double atan(double a);
    static float atan(float a);
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
};

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
Math::asin(float a) {
    return std::asin(a);
}

inline double
Math::asin(double a) {
    return std::asin(a);
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
Math::atan(float a) {
    return std::atan(a);
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
    return std::sqrt(a);
}

inline double
Math::sqrt(double a) {
    return std::sqrt(a);
}
}
#endif

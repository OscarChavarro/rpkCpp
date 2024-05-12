#ifndef __MATH__
#define __MATH__

namespace java {

class Math {
  public:
    static double abs(double a);
    static int min(int a, int b);
    static int max(int a, int b);
    static float min(float a, float b);
    static float max(float a, float b);
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

}

#endif

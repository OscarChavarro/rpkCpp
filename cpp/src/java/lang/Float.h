#ifndef __JAVA_FLOAT__
#define __JAVA_FLOAT__

namespace java {
class Float {
  public:
    static constexpr float MIN_VALUE = 1.40129846e-45f;
    static constexpr float MAX_VALUE = 3.40282347e+38f;

    static bool isFinite(float a);
};

}

#endif

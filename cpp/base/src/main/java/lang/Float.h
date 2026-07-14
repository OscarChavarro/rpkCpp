#ifndef JAVA_FLOAT__
#define JAVA_FLOAT__

namespace java {
class Float {
  public:
    static constexpr float MIN_VALUE = 1.40129846e-45F;
    static constexpr float MAX_VALUE = 3.40282347e+38F;

    static bool isFinite(float a);
};

}

#endif

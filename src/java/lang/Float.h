#ifndef __JAVA_FLOAT__
#define __JAVA_FLOAT__

#include <cmath>

namespace java {
class Float {
  public:
    static bool isFinite(float a);
};

inline bool
Float::isFinite(float a) {
    return std::isfinite(a);
}

}

#endif

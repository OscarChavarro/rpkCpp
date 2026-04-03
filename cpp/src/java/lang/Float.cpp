#include <cmath>

#include "java/lang/Float.h"

namespace java {

bool
Float::isFinite(float a) {
    return std::isfinite(a);
}

}

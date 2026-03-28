#include "java/lang/Float.h"

#include <cmath>

namespace java {

bool
Float::isFinite(float a) {
    return std::isfinite(a);
}

}

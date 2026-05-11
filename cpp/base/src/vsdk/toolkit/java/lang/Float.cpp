#include <cmath>

#include "vsdk/toolkit/java/lang/Float.h"

namespace java {

bool
Float::isFinite(float a) {
    return std::isfinite(a);
}

}

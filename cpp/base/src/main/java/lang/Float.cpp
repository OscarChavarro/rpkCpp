#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "java/lang/Double.h"
#include "java/lang/Float.h"
namespace java {

bool
Float::isFinite(float a) {
    return std::isfinite(a);
}

float
Float::parseFloat(const java::String& text) {
    return static_cast<float>(Double::parseDouble(text));
}

java::String
Float::toString(float value) {
    if ( !std::isfinite(value) || value == 0.0f ) {
        return Double::toString(static_cast<double>(value));
    }
    // Shortest precision that reads back as the same float, then written
    // as the double with that decimal text (0.1f is "0.1", not
    // "0.10000000149011612")
    char buffer[64];
    for ( int precision = 1; precision <= 9; precision++ ) {
        std::snprintf(buffer, sizeof(buffer), "%.*g", precision,
                      static_cast<double>(value));
        if ( std::strtof(buffer, nullptr) == value ) {
            break;
        }
    }
    return Double::toString(std::strtod(buffer, nullptr));
}

}

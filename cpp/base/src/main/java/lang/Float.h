#ifndef __FLOAT__
#define __FLOAT__

#include "java/lang/String.h"

namespace java {
class Float {
  public:
    static constexpr float MIN_VALUE = 1.40129846e-45F;
    static constexpr float MAX_VALUE = 3.40282347e+38F;

    static bool isFinite(float a);

    /**
    Emulation of `Float.parseFloat`: leading and trailing white space is
    ignored.
    @param text text with a decimal number
    @return the number
    @throws NumberFormatException if the text is not a number
    */
    static float parseFloat(const java::String& text);

    /**
    Emulation of `Float.toString`: the shortest decimal text that reads
    back as the same float, written as `Double::toString` does ("0.1",
    "1.0E7").
    @param value number to write
    @return the text of the number
    */
    static java::String toString(float value);
};

}

#endif

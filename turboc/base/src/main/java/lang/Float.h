#ifndef __FLOAT__
#define __FLOAT__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"

class Float {
  public:
    static const float MIN_VALUE;
    static const float MAX_VALUE;

    static BOOLEAN isFinite(float a);

    /**
    Emulation of `Float.parseFloat`: leading and trailing white space is
    ignored.
    @param text text with a decimal number
    @return the number
    @throws NumberFormatException if the text is not a number
    */
    static float parseFloat(const String& text);

    /**
    Emulation of `Float.toString`: the shortest decimal text that reads
    back as the same float, written as `Double::toString` does ("0.1",
    "1.0E7").
    @param value number to write
    @return the text of the number
    */
    static String toString(float value);
};


#endif

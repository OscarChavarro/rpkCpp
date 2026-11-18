#ifndef __INTEGER__
#define __INTEGER__

#include "java/lang/String.h"

namespace java {

class Integer {
  public:
    static const int MIN_VALUE = (-2147483647 - 1);
    static const int MAX_VALUE = 2147483647;

    /**
    Emulation of `Integer.parseInt` (base 10).
    @param text text with an integer number, with an optional sign
    @return the number
    @throws NumberFormatException if the text is not an integer in range
    */
    static int parseInt(const java::String& text);
};

}

#endif

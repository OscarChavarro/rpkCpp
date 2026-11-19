#ifndef __INTEGER__
#define __INTEGER__

#include "java/lang/String.h"


class Integer {
  public:
    static const int MIN_VALUE;
    static const int MAX_VALUE;

    /**
    Emulation of `Integer.parseInt` (base 10).
    @param text text with an integer number, with an optional sign
    @return the number
    @throws NumberFormatException if the text is not an integer in range
    */
    static int parseInt(const String& text);
};


#endif

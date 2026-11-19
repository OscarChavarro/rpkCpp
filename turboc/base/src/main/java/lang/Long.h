#ifndef __LONG__
#define __LONG__

#include "java/lang/String.h"


class Long {
  public:
    static const long long MIN_VALUE;
    static const long long MAX_VALUE;

    /**
    Emulation of `Long.parseLong` (base 10).
    @param text text with an integer number, with an optional sign
    @return the number
    @throws NumberFormatException if the text is not an integer in range
    */
    static long long parseLong(const String& text);
};


#endif

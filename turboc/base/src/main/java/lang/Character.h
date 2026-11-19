#ifndef __CHARACTER__
#define __CHARACTER__

#include "vsdk/toolkit/common/VSDK.h"


class Character {
  public:
    static BOOLEAN isDigit(int value);
    static BOOLEAN isSpace(int value);
    static BOOLEAN isLetter(int value);
};


#endif

#ifndef __CHARACTER__
#define __CHARACTER__

#include "common/VSDK.h"


class Character {
  public:
    static bool isDigit(int value);
    static bool isSpace(int value);
    static bool isLetter(int value);
};


#endif

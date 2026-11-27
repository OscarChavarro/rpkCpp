#ifndef __ENUM_APP_OPTIONS__
#define __ENUM_APP_OPTIONS__

#include "vsdk/toolkit/common/VSDK.h"

class EnumAppOptions {
  public:
    int width;
    int height;
    int nqcdivs;
    int yesValue;
    int noValue;
    int debug;

    EnumAppOptions(int widthValue, int heightValue, int nqcdivsValue,
                   int yes, int no, int debugValue):
        width(widthValue), height(heightValue), nqcdivs(nqcdivsValue),
        yesValue(yes), noValue(no), debug(debugValue) {}
};

#endif

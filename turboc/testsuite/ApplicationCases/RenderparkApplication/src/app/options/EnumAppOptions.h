#ifndef __APP_OPTIONS_CONTEXT__
#define __APP_OPTIONS_CONTEXT__

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

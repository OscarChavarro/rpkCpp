#ifndef __LUMINANCE_AREA__
#define __LUMINANCE_AREA__

#include "vsdk/toolkit/common/VSDK.h"

/**
Stores luminance-area pairs for median area-weighted luminance
selection.
*/
class LuminanceArea {
  public:
    double luminance;
    double area;
};

#endif

#ifndef __PIXEL_FILTER__
#define __PIXEL_FILTER__

#include "common/VSDK.h"

class PixelFilter {
  public:
    PixelFilter();
    virtual ~PixelFilter();

    virtual void sample(double *xi1, double *xi2) = 0;
};

#endif

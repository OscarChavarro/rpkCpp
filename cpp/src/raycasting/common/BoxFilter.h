#ifndef _BOX_FILTER__
#define _BOX_FILTER__

#include "raycasting/common/PixelFilter.h"

class BoxFilter final : public PixelFilter {
  public:
    BoxFilter();
    ~BoxFilter() final;

    void sample(double *, double *) final;
};

#endif

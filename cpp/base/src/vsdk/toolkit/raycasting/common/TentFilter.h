#ifndef TENT_FILTER__
#define TENT_FILTER__

#include "vsdk/toolkit/raycasting/common/PixelFilter.h"

class TentFilter final : public PixelFilter {
  public:
    TentFilter();
    ~TentFilter() final;

    void sample(double *, double *) final;
};

#endif

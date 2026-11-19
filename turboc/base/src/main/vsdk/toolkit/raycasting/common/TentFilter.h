#ifndef __TENT_FILTER__
#define __TENT_FILTER__

#include "vsdk/toolkit/raycasting/common/PixelFilter.h"

class TentFilter: public PixelFilter{ public:
    TentFilter();
    ~TentFilter();

    void sample(double *, double *);
};

#endif

#ifndef __TENT_FILTER__
#define __TENT_FILTER__

#include "raycasting/common/PixelFilter.h"

class TentFilter: public PixelFilter{ public:
    TentFilter();
    ~TentFilter();

    void sample(double *, double *);
};

#endif

#ifndef ___BOX_FILTER__
#define ___BOX_FILTER__

#include "vsdk/toolkit/raycasting/common/PixelFilter.h"

class BoxFilter: public PixelFilter{ public:
    BoxFilter();
    ~BoxFilter();

    void sample(double *, double *);
};

#endif

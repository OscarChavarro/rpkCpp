#ifndef __NORMAL_FILTER__
#define __NORMAL_FILTER__

#include "raycasting/common/PixelFilter.h"

class NormalFilter: public PixelFilter{ public:
    double sigma;
    double dist;

    explicit NormalFilter(double s = 0.70710678, double d = 2.0);
    ~NormalFilter();

    void sample(double *, double *);
};

#endif

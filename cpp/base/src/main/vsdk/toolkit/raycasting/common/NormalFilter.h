#ifndef NORMAL_FILTER__
#define NORMAL_FILTER__

#include "vsdk/toolkit/raycasting/common/PixelFilter.h"

class NormalFilter final : public PixelFilter {
public:
    double sigma;
    double dist;

    explicit NormalFilter(double s = 0.70710678, double d = 2.0);
    ~NormalFilter() final;

    void sample(double *, double *) final;
};

#endif

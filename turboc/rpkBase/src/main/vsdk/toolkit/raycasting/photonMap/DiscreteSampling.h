#ifndef __SAMPLING__
#define __SAMPLING__

#include "vsdk/toolkit/common/VSDK.h"

class DiscreteSampling {
  public:
    static int
    sample(
        const double probabilities[],
        double total,
        double *x1,
        double *probabilityDensityFunction);
};

#endif

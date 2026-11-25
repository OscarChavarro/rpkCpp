#ifndef __DISCRETE_SAMPLING__
#define __DISCRETE_SAMPLING__

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

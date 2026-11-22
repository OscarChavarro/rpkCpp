#ifndef SAMPLING__
#define SAMPLING__

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

#ifndef STOCHASTIC_RADIOSITY_GALERKIN_BASIS__
#define STOCHASTIC_RADIOSITY_GALERKIN_BASIS__

/**
All bases are orthonormal on their standard domain
*/
class GalerkinBasis {
  public:
    static constexpr int MAX_BASIS_SIZE = 10;
    using FILTER = double[MAX_BASIS_SIZE][MAX_BASIS_SIZE];
    using FILTER_TABLE = FILTER[4];

    const char *description;
    int size; // Number of basis functions

    // function[alpha](u,v) evaluates \alpha-th basis function at (u,v)
    double (**function)(double u, double v);

    // Same, but evaluated dual basis function (on standard domain)
    double (**dualFunction)(double u, double v);

    // Push-pull filter coefficients for regular quadtree subdivision.
    // regular_filter[sigma][alpha][beta] is the filter coefficient
    // relating basis function alpha on the parent element with
    // basis function beta on the regular sub-element with index
    // sigma. See pushRadiance() and pullRadiance()
    FILTER_TABLE *regularFilter;
};

#endif

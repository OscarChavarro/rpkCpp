/**
Orthonormal basis for the standard triangle (0, 0), (1, 0), (0, 1)
*/

#ifndef __BASISTRIMCRAD__
#define __BASISTRIMCRAD__

#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityBasis.h"

class Basistrimcrad final {
  public:
    static StochasticRadiosityBasis createBasis();
    static double tm0(double u, double v);
    static double tm1(double u, double v);
    static double tm2(double u, double v);
    static double tm3(double u, double v);
    static double tm4(double u, double v);
    static double tm5(double u, double v);
    static double tm6(double u, double v);
    static double tm7(double u, double v);
    static double tm8(double u, double v);
    static double tm9(double u, double v);

  private:
    using BasisFunction = double (*)(double, double);
    static BasisFunction f[StochasticRadiosityBasis::MAX_BASIS_SIZE];
    static StochasticRadiosityBasis::FILTER_TABLE h;
    static StochasticRadiosityBasis stochasticRadiosityCreateTriBasis();
};

#endif

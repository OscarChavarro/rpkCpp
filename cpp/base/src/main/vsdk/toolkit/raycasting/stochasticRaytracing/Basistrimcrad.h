/**
Orthonormal basis for the standard triangle (0, 0), (1, 0), (0, 1)
*/

#ifndef BASIS_TRI_MCRAD__
#define BASIS_TRI_MCRAD__

#include "vsdk/toolkit/raycasting/stochasticRaytracing/GalerkinBasis.h"

class Basistrimcrad final {
  public:
    static GalerkinBasis createBasis();
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
    static BasisFunction f[GalerkinBasis::MAX_BASIS_SIZE];
    static GalerkinBasis::FILTER_TABLE h;
    static GalerkinBasis stochasticRadiosityCreateTriBasis();
};

#endif

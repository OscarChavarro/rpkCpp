#ifndef BASIS_QUAD_GALERKIN__
#define BASIS_QUAD_GALERKIN__

#include "vsdk/toolkit/galerkin/GalerkinBasis.h"

class BasisQuadGalerkin final {
  public:
    static GalerkinBasis &instance();

  private:
    BasisQuadGalerkin() = delete;

    static double qg0(double u, double v);
    static double qg1(double u, double v);
    static double qg2(double u, double v);
    static double qg3(double u, double v);
    static double qg4(double u, double v);
    static double qg5(double u, double v);
    static double qg6(double u, double v);
    static double qg7(double u, double v);
    static double qg8(double u, double v);
    static double qg9(double u, double v);
};

#endif

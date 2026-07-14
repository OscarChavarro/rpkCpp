#ifndef BASIS_TRI_GALERKIN__
#define BASIS_TRI_GALERKIN__

#include "vsdk/toolkit/galerkin/GalerkinBasis.h"

class BasisTriGalerkin final {
  public:
    static GalerkinBasis &instance();

  private:
    BasisTriGalerkin() = delete;

    static double tg0(double u, double v);
    static double tg1(double u, double v);
    static double tg2(double u, double v);
    static double tg3(double u, double v);
    static double tg4(double u, double v);
    static double tg5(double u, double v);
    static double tg6(double u, double v);
    static double tg7(double u, double v);
    static double tg8(double u, double v);
    static double tg9(double u, double v);
};

#endif

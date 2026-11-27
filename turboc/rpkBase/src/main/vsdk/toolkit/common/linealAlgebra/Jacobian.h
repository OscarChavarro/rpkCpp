#ifndef __JACOBIAN__
#define __JACOBIAN__

#include "vsdk/toolkit/common/VSDK.h"

/**
Jacobian for a quadrilateral patch is J(u, v) = A + B.u + C.v
*/
class Jacobian {
  public:
    double A;
    double B;
    double C;

    explicit Jacobian(double inA, double inB, double inC);
    virtual ~Jacobian();
};

#endif

#include "skin/Jacobian.h"

Jacobian *
jacobianCreate(float A, float B, float C) {
    Jacobian *jacobian = new Jacobian();
    jacobian->A = A;
    jacobian->B = B;
    jacobian->C = C;

    return jacobian;
}

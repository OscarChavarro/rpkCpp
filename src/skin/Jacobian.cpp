#include <cstdlib>

#include "skin/Jacobian.h"

JACOBIAN *
JacobianCreate(float A, float B, float C) {
    JACOBIAN *jacobian = (JACOBIAN *)malloc(sizeof(JACOBIAN));
    jacobian->A = A;
    jacobian->B = B;
    jacobian->C = C;

    return jacobian;
}

void
JacobianDestroy(JACOBIAN *jacobian) {
    free(jacobian);
}

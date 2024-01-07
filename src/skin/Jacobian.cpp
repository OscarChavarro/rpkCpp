#include <cstdlib>

#include "skin/Jacobian.h"

Jacobian *
jacobianCreate(float A, float B, float C) {
    Jacobian *jacobian = (Jacobian *)malloc(sizeof(Jacobian));
    jacobian->A = A;
    jacobian->B = B;
    jacobian->C = C;

    return jacobian;
}

void
jacobianDestroy(Jacobian *jacobian) {
    free(jacobian);
}

#ifndef __GALERKIN_ITERATION_METHOD__
#define __GALERKIN_ITERATION_METHOD__

/**
Reference:
[COHE1993] Cohen, M. Wallace, J. "Radiosity and Realistic Image Synthesis",
     Academic Press Professional, 1993.
*/

// See [COHE1993] section 5.3.
enum GalerkinIterationMethod {
    JACOBI,
    GAUSS_SEIDEL,
    SOUTH_WELL
};

#endif

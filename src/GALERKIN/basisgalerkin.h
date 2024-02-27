/**
Higher order approximations for Galerkin radiosity
*/

#ifndef __BASIS__
#define __BASIS__

#include "GALERKIN/GalerkinElement.h"

// No basis consists of more than this number of basis functions
#define MAX_BASIS_SIZE 10

/**
All bases are orthonormal on their standard domain
*/
class GalerkinBasis {
  public:
    const char *description; // For debugging
    int size; // Number of basis functions

    // function[alpha](u,v) evaluates phi_\alpha at (u, v)
    double (*function[MAX_BASIS_SIZE])(double u, double v);

    // Push-pull filter coefficients for regular subdivision.
    // regular_filter[sigma][alpha][beta] is the filter coefficient
    // relating basis function alpha on the parent element with
    // basis function beta on the regular sub-element with index
    // sigma. See PushRadiance() and PullRadiance() in basis.c
    double regular_filter[4][MAX_BASIS_SIZE][MAX_BASIS_SIZE];
};

extern GalerkinBasis GLOBAL_galerkin_quadBasis;
extern GalerkinBasis GLOBAL_galerkin_triBasis;

extern COLOR
basisGalerkinRadianceAtPoint(
    GalerkinElement *elem,
    COLOR *coefficients,
    double u,
    double v);

extern void
basisGalerkinPush(
    GalerkinElement *parent,
    COLOR *parent_coefficients,
    GalerkinElement *child,
    COLOR *child_coefficients);

extern void basisGalerkinInitBasis();

#endif

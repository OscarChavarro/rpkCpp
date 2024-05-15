/**
Higher order approximations for Galerkin radiosity
*/

#ifndef __BASIS__
#define __BASIS__

#include "common/ColorRgb.h"
#include "raycasting/stochasticRaytracing/mcrad.h"

// No basis consists of more than this number of basis functions
#define MAX_BASIS_SIZE 10

typedef double FILTER[MAX_BASIS_SIZE][MAX_BASIS_SIZE];
typedef FILTER FILTER_TABLE[4];

/**
All bases are orthonormal on their standard domain
*/
class GalerkinBasis {
  public:
    const char *description;
    int size; // Number of basis functions

    // function[alpha](u,v) evaluates \alpha-th basis function at (u,v)
    double (**function)(double u, double v);

    // Same, but evaluated dual basis function (on standard domain)
    double (**dualFunction)(double u, double v);

    /* push-pull filter coefficients for regular quadtree subdivision.
     * regular_filter[sigma][alpha][beta] is the filter coefficient
     * relating basis function alpha on the parent element with
     * basis function beta on the regular sub-element with index
     * sigma. See PushRadiance() and PullRadiance() in basis.c. */
    FILTER_TABLE *regularFilter;
};

// Bases for quadrilaterals and triangles, implemented in basis[quad|tri].cpp
extern GalerkinBasis GLOBAL_stochasticRadiosity_triBasis;
extern GalerkinBasis GLOBAL_stochasticRadiosity_quadBasis;
extern GalerkinBasis GLOBAL_stochasticRadiosity_clusterBasis;

#define NR_APPROX_TYPES 5
enum StochasticRaytracingApproximation {
    CONSTANT = 0, // 1
    LINEAR = 1, // 1, u, v
    BI_LINEAR = 2, // 1, u, v, uv
    QUADRATIC = 3, // 1, u, v, uv, u2, v2
    CUBIC = 4 // 1, u, v, uv, u2, v2, u3, u2v, uv2, v3
};

// Description of the approximation types
class ApproximationTypeDescription {
  public:
    const char *name;
    int basis_size;
};

extern ApproximationTypeDescription GLOBAL_stochasticRadiosity_approxDesc[NR_APPROX_TYPES];

#define NR_ELEMENT_TYPES 2
enum ElementType {
    ET_TRIANGLE = 0,
    ET_QUAD = 1
};

// Orthonormal canonical basis of given order for given type of elements
extern GalerkinBasis GLOBAL_stochasticRadiosity_basis[NR_ELEMENT_TYPES][NR_APPROX_TYPES];
extern GalerkinBasis GLOBAL_stochasticRadiosity_dummyBasis;

extern void monteCarloRadiosityInitBasis();
extern ColorRgb colorAtUv(GalerkinBasis *basis, ColorRgb *rad, double u, double v);
extern void filterColorDown(ColorRgb *parent, FILTER *h, ColorRgb *child, int n);
extern void filterColorUp(ColorRgb *child, FILTER *h, ColorRgb *parent, int n, double areaFactor);

#endif

/* basis.h: higher order approximations for Galerkin radiosity */

#ifndef __BASIS__
#define __BASIS__

#include "material/color.h"
#include "raycasting/stochasticRaytracing/mcrad.h"

/* no basis consists of more than this number of basis functions */
#define MAX_BASIS_SIZE    10

typedef double FILTER[MAX_BASIS_SIZE][MAX_BASIS_SIZE];
typedef FILTER FILTERTABLE[4];

/* all bases are orthonormal on their standard domain. */
class GalerkinBasis {
  public:
    const char *description; // for debugging

    int size; // number of basis functions

    // function[alpha](u,v) evaluates \alpha-th basis function at (u,v)
    double (**function)(double u, double v);

    /* same, but evaluated dual basis function (on standard domain) */
    double (**dualfunction)(double u, double v);

    /* push-pull filter coefficients for regular quadtree subdivision.
     * regular_filter[sigma][alpha][beta] is the filter coefficient
     * relating basis function alpha on the parent element with
     * basis function beta on the regular subelement with index
     * sigma. See PushRadiance() and PullRadiance() in basis.c. */
    FILTERTABLE *regular_filter;
};

/* bases for quadrilaterals and triangles, implemented in basis[quad|tri].c */
extern GalerkinBasis GLOBAL_stochasticRadiosisty_triBasis;
extern GalerkinBasis GLOBAL_stochasticRadiosisty_quadBasis;
extern GalerkinBasis GLOBAL_stochasticRadiosisty_clusterBasis;

#define NR_APPROX_TYPES 5
enum APPROX_TYPE {
    AT_CONSTANT = 0,    /* 1 */
    AT_LINEAR = 1,    /* 1, u, v */
    AT_BILINEAR = 2,    /* 1, u, v, uv */
    AT_QUADRATIC = 3,    /* 1, u, v, uv, u2, v2 */
    AT_CUBIC = 4    /* 1, u, v, uv, u2, v2, u3, u2v, uv2, v3 */
};

/* description of the approximation types */
class APPROXDESC {
  public:
    const char *name;
    int basis_size;
};

extern APPROXDESC GLOBAL_stochasticRadiosisty_approxDesc[NR_APPROX_TYPES];

#define NR_ELEMENT_TYPES 2
enum ELEMENT_TYPE {
    ET_TRIANGLE = 0,
    ET_QUAD = 1
};

/* orthonormal canonical basis of given order for given type of elements */
extern GalerkinBasis GLOBAL_stochasticRadiosisty_basis[NR_ELEMENT_TYPES][NR_APPROX_TYPES], GLOBAL_stochasticRadiosisty_dummyBasis;

extern void monteCarloRadiosityInitBasis();
extern void printBasis(GalerkinBasis *basis);
extern COLOR colorAtUv(GalerkinBasis *basis, COLOR *rad, double u, double v);
extern void filterColorDown(COLOR *parent, FILTER *h, COLOR *child, int n);
extern void filterColorUp(COLOR *child, FILTER *h, COLOR *parent, int n, double areafactor);

#endif

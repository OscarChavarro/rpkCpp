/* basis.h: higher order approximations for Galerkin radiosity */

#ifndef _BASIS_H_
#define _BASIS_H_

#define InitBasis GAL_InitBasis

#include "GALERKIN/elementgalerkin.h"

/* no basis consists of more than this number of basis functions */
#define MAXBASISSIZE    10

/* all bases are orthonormal on their standard domain. */
class GalerkinBasis {
  public:
    const char *description;    /* for debugging */

    int size;    /* number of basis functions */

    /* function[alpha](u,v) evaluates phi_\alpha at (u,v). */
    double (*function[MAXBASISSIZE])(double u, double v);

    /* push-pull filter coefficients for regular subdivision.
     * regular_filter[sigma][alpha][beta] is the filter coefficient
     * relating basis function alpha on the parent element with
     * basis function beta on the regular subelement with index
     * sigma. See PushRadiance() and PullRadiance() in basis.c. */
    double regular_filter[4][MAXBASISSIZE][MAXBASISSIZE];
};

extern GalerkinBasis quadBasis, triBasis;

extern COLOR RadianceAtPoint(ELEMENT *elem, COLOR *coefficients, double u, double v);

extern void Push(ELEMENT *parent, COLOR *parent_coefficients,
                 ELEMENT *child, COLOR *child_coefficients);

extern void Pull(ELEMENT *parent, COLOR *parent_coefficients,
                 ELEMENT *child, COLOR *child_coefficients);

extern void InitBasis();

#endif

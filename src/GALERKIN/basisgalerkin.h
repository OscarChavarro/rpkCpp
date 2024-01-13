/**
Higher order approximations for Galerkin radiosity
*/

#ifndef _BASIS_H_
#define _BASIS_H_

#include "GALERKIN/GalerkinElement.h"

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

extern GalerkinBasis GLOBAL_galerkin_quadBasis;
extern GalerkinBasis GLOBAL_galerkin_triBasis;

extern COLOR basisGalerkinRadianceAtPoint(GalerkingElement *elem, COLOR *coefficients, double u, double v);

extern void
basisGalerkinPush(
        GalerkingElement *parent,
        COLOR *parent_coefficients,
        GalerkingElement *child,
        COLOR *child_coefficients);

extern void
basisGalerkinPull(
        GalerkingElement *parent,
        COLOR *parent_coefficients,
        GalerkingElement *child,
        COLOR *child_coefficients);

extern void basisGalerkinInitBasis();

#endif

/**
Higher order approximations for Galerkin radiosity
*/

#ifndef __BASIS__
#define __BASIS__

#include "galerkin/GalerkinElement.h"
#include "galerkin/GalerkinState.h"

/**
All bases are orthonormal on their standard domain
*/
class GalerkinBasis {
  public:
    static constexpr int MAX_BASIS_SIZE = 10;

    const char *description; // For debugging
    int size; // Number of basis functions

    // function[alpha](u,v) evaluates phi_\alpha at (u, v)
    double (*function[MAX_BASIS_SIZE])(double u, double v);

    // Push-pull filter coefficients for regular subdivision.
    // regular_filter[sigma][alpha][beta] is the filter coefficient
    // relating basis function alpha on the parent element with
    // basis function beta on the regular sub-element with index
    // sigma. See PushRadiance() and PullRadiance() in basis.c
    double regularFilter[4][MAX_BASIS_SIZE][MAX_BASIS_SIZE];

    static ColorRgb
    radianceAtPoint(
        const GalerkinElement *element,
        const ColorRgb *coefficients,
        double u,
        double v);

    static void
    push(
        const GalerkinElement *element,
        const ColorRgb *parentCoefficients,
        const GalerkinElement *child,
        ColorRgb *childCoefficients);

    static void pushPullRadiance(GalerkinElement *top, GalerkinState *galerkinState);

    static void
    computeRegularFilterCoefficients(
        GalerkinBasis *basis,
        const Matrix2x2 upTransform[],
        const CubatureRule *cubaRule);

    static const GalerkinBasis *basisForVertexCount(int numberOfVertices);
    static GalerkinBasis *mutableBasisForVertexCount(int numberOfVertices);

  private:
    static void
    pull(
        const GalerkinElement *parent,
        ColorRgb *parentCoefficients,
        const GalerkinElement *child,
        const ColorRgb *childCoefficients);

    static void
    pushPullRadianceRecursive(
        GalerkinElement *element,
        ColorRgb *Bdown,
        ColorRgb *Bup,
        GalerkinState *galerkinState);

    static void
    computeFilterCoefficients(
        const GalerkinBasis *parentBasis,
        int parentSize,
        const GalerkinBasis *childBasis,
        int childSize,
        const Matrix2x2 *upTransform,
        const CubatureRule *cubatureRule,
        double filter[MAX_BASIS_SIZE][MAX_BASIS_SIZE]);
};

#endif

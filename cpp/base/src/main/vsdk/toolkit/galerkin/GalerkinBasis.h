/**
Higher order approximations for Galerkin radiosity
*/

#ifndef BASIS__
#define BASIS__

#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/galerkin/GalerkinState.h"

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

    static ColorRgbMutable
    radianceAtPoint(
        const GalerkinElement *element,
        const ColorRgbMutable *coefficients,
        double u,
        double v);

    static void
    push(
        const GalerkinElement *element,
        const ColorRgbMutable *parentCoefficients,
        const GalerkinElement *child,
        ColorRgbMutable *childCoefficients);

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
        ColorRgbMutable *parentCoefficients,
        const GalerkinElement *child,
        const ColorRgbMutable *childCoefficients);

    static void
    pushPullRadianceRecursive(
        GalerkinElement *element,
        ColorRgbMutable *Bdown,
        ColorRgbMutable *Bup,
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

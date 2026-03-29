/**
Higher order approximations for Galerkin radiosity
*/

#ifndef __BASIS__
#define __BASIS__

#include "common/ColorRgb.h"
#include "raycasting/stochasticRaytracing/Mcrad.h"
#include "raycasting/stochasticRaytracing/GalerkinBasis.h"
#include "raycasting/stochasticRaytracing/ApproximationTypeDescription.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElementType.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingApproximation.h"

class CubatureRule;

// Bases for quadrilaterals and triangles, implemented in basis[quad|tri].cpp
extern GalerkinBasis GLOBAL_stochasticRadiosity_triBasis;
extern GalerkinBasis GLOBAL_stochasticRadiosity_quadBasis;
extern GalerkinBasis GLOBAL_stochasticRadiosity_clusterBasis;

constexpr int NUMBER_OF_APPROXIMATION_TYPES = 5;

extern ApproximationTypeDescription GLOBAL_stochasticRadiosity_approxDesc[NUMBER_OF_APPROXIMATION_TYPES];

// Orthonormal canonical basis of given order for given type of elements
extern GalerkinBasis GLOBAL_stochasticRadiosity_basis[NUMBER_OF_ELEMENT_TYPES][NUMBER_OF_APPROXIMATION_TYPES];
extern GalerkinBasis GLOBAL_stochasticRadiosity_dummyBasis;

class Basismcrad final {
  public:
    static void monteCarloRadiosityInitBasis();
    static ColorRgb colorAtUv(const GalerkinBasis *basis, const ColorRgb *rad, double u, double v);
    static void filterColorDown(const ColorRgb *parent, FILTER *h, ColorRgb *child, int n);
    static void filterColorUp(const ColorRgb *child, FILTER *h, ColorRgb *parent, int n, double areaFactor);
    static double oneBasis(double u, double v);

  private:
    static GalerkinBasis makeBasis(StochasticRadiosityElementType et, StochasticRaytracingApproximation at);
    static void computeFilterCoefficients(
        const GalerkinBasis *parentBasis,
        int parentSize,
        const GalerkinBasis *childBasis,
        int childSize,
        const Matrix2x2 *upTransform,
        const CubatureRule *cubatureRule,
        FILTER *filter);
    static void basisGalerkinComputeRegularFilterCoefficients(
        GalerkinBasis *basis,
        const Matrix2x2 *upTransform,
        const CubatureRule *cubatureRule);
};

#endif

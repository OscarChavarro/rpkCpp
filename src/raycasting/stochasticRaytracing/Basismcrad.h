/**
Higher order approximations for Galerkin radiosity
*/

#ifndef __BASIS__
#define __BASIS__

#include "common/ColorRgb.h"
#include "common/linealAlgebra/Matrix2x2.h"
#include "raycasting/stochasticRaytracing/Mcrad.h"
#include "raycasting/stochasticRaytracing/GalerkinBasis.h"
#include "raycasting/stochasticRaytracing/ApproximationTypeDescription.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElementType.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingApproximation.h"

class CubatureRule;

class StochasticRadiosityBasisState {
  public:
    static constexpr int NUMBER_OF_APPROXIMATION_TYPES = 5;

    ApproximationTypeDescription approxDesc[NUMBER_OF_APPROXIMATION_TYPES];
    GalerkinBasis basis[StochasticRadiosityElementTypeInfo::NUMBER_OF_ELEMENT_TYPES][NUMBER_OF_APPROXIMATION_TYPES];
    GalerkinBasis triBasis;
    GalerkinBasis quadBasis;
    GalerkinBasis dummyBasis;
    GalerkinBasis clusterBasis;
    Matrix2x2 quadUpTransform[4];
    Matrix2x2 triangleUpTransform[4];
    bool inited;

    StochasticRadiosityBasisState();
    static void setActiveState(StochasticRadiosityBasisState &state);
    static StochasticRadiosityBasisState &activeState();

  private:
    static double (*oneBasisTable[1])(double, double);
    static GalerkinBasis stochasticRadiosityCreateQuadBasis();
    static StochasticRadiosityBasisState *&activeStatePtr();
    static Matrix2x2 createTransform(float m00, float m01, float m10, float m11, float t0, float t1);
};

class Basismcrad final {
  public:
    static void monteCarloRadiosityInitBasis();
    static ColorRgb colorAtUv(const GalerkinBasis *basis, const ColorRgb *rad, double u, double v);
    static void filterColorDown(const ColorRgb *parent, GalerkinBasis::FILTER *h, ColorRgb *child, int n);
    static void filterColorUp(const ColorRgb *child, GalerkinBasis::FILTER *h, ColorRgb *parent, int n, double areaFactor);
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
        GalerkinBasis::FILTER *filter);
    static void basisGalerkinComputeRegularFilterCoefficients(
        GalerkinBasis *basis,
        const Matrix2x2 *upTransform,
        const CubatureRule *cubatureRule);
};

#endif

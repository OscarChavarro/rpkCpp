/**
Higher order approximations for Galerkin radiosity
*/

#ifndef __BASISMCRAD__
#define __BASISMCRAD__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix2x2.h"
#include "vsdk/toolkit/numericalAnalysis/CubatureRule.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Mcrad.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityBasis.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/ApproximationTypeDescription.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElementType.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRaytracingApproximation.h"

class StochasticRadiosityBasisState {
  public:
    static constexpr int NUMBER_OF_APPROXIMATION_TYPES = 5;

    ApproximationTypeDescription approxDesc[NUMBER_OF_APPROXIMATION_TYPES];
    StochasticRadiosityBasis basis[StochasticRadiosityElementTypeInfo::NUMBER_OF_ELEMENT_TYPES][NUMBER_OF_APPROXIMATION_TYPES];
    StochasticRadiosityBasis triBasis;
    StochasticRadiosityBasis quadBasis;
    StochasticRadiosityBasis dummyBasis;
    StochasticRadiosityBasis clusterBasis;
    Matrix2x2 quadUpTransform[4];
    Matrix2x2 triangleUpTransform[4];
    bool inited;

    StochasticRadiosityBasisState();
    static void setActiveState(StochasticRadiosityBasisState &state);
    static StochasticRadiosityBasisState &activeState();

  private:
    using BasisFunction = double (*)(double, double);
    static BasisFunction oneBasisTable[1];
    static StochasticRadiosityBasis stochasticRadiosityCreateQuadBasis();
    static StochasticRadiosityBasisState *&activeStatePtr();
    static Matrix2x2 createTransform(float m00, float m01, float m10, float m11, float t0, float t1);
};

class Basismcrad final {
  public:
    static void monteCarloRadiosityInitBasis();
    static ColorRgb colorAtUv(const StochasticRadiosityBasis *basis, const ColorRgb *rad, double u, double v);
    static void filterColorDown(const ColorRgb *parent, StochasticRadiosityBasis::FILTER *h, ColorRgb *child, int n);
    static void filterColorUp(const ColorRgb *child, StochasticRadiosityBasis::FILTER *h, ColorRgb *parent, int n, double areaFactor);
    static double oneBasis(double u, double v);

  private:
    static StochasticRadiosityBasis makeBasis(StochasticRadiosityElementType et, StochasticRaytracingApproximation at);
    static void computeFilterCoefficients(
        const StochasticRadiosityBasis *parentBasis,
        int parentSize,
        const StochasticRadiosityBasis *childBasis,
        int childSize,
        const Matrix2x2 *upTransform,
        const CubatureRule *cubatureRule,
        StochasticRadiosityBasis::FILTER *filter);
    static void basisGalerkinComputeRegularFilterCoefficients(
        StochasticRadiosityBasis *basis,
        const Matrix2x2 *upTransform,
        const CubatureRule *cubatureRule);
};

#endif

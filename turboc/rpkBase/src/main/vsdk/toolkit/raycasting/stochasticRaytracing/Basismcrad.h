#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

/**
Higher order approximations for Galerkin radiosity
*/

#ifndef __BASISMCRAD__
#define __BASISMCRAD__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix2x2.h"
#include "vsdk/toolkit/numericalAnalysis/CubatureRule.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Mcrad.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/GalerkinBasis.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/ApproximationTypeDescription.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElementType.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRaytracingApproximation.h"

#define STOCH_RAD_BASIS_NUMBER_OF_APPROXIMATION_TYPES 5

class StochasticRadiosityBasisState{ public:
    ApproximationTypeDescription approxDesc[STOCH_RAD_BASIS_NUMBER_OF_APPROXIMATION_TYPES];
    GalerkinBasis basis[StochRadElemTypeInfo::NUMBER_OF_ELEMENT_TYPES][STOCH_RAD_BASIS_NUMBER_OF_APPROXIMATION_TYPES];
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
    typedef double (*BasisFunction)(double, double);
    static BasisFunction oneBasisTable[1];
    static GalerkinBasis stchsRadCreateQuadBasis();
    static StochasticRadiosityBasisState *&activeStatePtr();
    static Matrix2x2 createTransform(double m00, double m01, double m10, double m11, double t0, double t1);
};

class Basismcrad{ public:
    static void monteCarloRadiosityInitBasis();
    static ColorRgbMutable colorAtUv(const GalerkinBasis *basis, const ColorRgbMutable *rad, double u, double v);
    static void filterColorDown(const ColorRgbMutable *parent, GalerkinBasis::FILTER *h, ColorRgbMutable *child, int n);
    static void filterColorUp(const ColorRgbMutable *child, GalerkinBasis::FILTER *h, ColorRgbMutable *parent, int n, double areaFactor);
    static double oneBasis(double u, double v);

  private:
    static GalerkinBasis makeBasis(StochasticRadiosityElementType et, StochRaytrApprx at);
    static void computeFilterCoefficients( const GalerkinBasis *parentBasis, int parentSize, const GalerkinBasis *childBasis, int childSize, const Matrix2x2 *upTransform, const CubatureRule *cubatureRule, GalerkinBasis::FILTER *filter);
    static void bssGalCompRegFiltCoeff( GalerkinBasis *basis, const Matrix2x2 *upTransform, const CubatureRule *cubatureRule);
};

#endif

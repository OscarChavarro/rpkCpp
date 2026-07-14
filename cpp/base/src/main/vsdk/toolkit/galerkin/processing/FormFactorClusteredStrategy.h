#ifndef FORM_FACTOR_CLUSTERED_STRATEGY__
#define FORM_FACTOR_CLUSTERED_STRATEGY__

#include "vsdk/toolkit/common/linealAlgebra/Ray.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/numericalAnalysis/CubatureRule.h"
#include "vsdk/toolkit/galerkin/Interaction.h"
#include "vsdk/toolkit/galerkin/ShadowCache.h"

class FormFactorClusteredStrategy {
  public:
    static void
    doConstantAreaToAreaFormFactor(
        Interaction *link,
        const CubatureRule *cubatureRuleRcv,
        const CubatureRule *cubatureRuleSrc,
        double Gxy[CubatureRule::MAXIMUM_NODES][CubatureRule::MAXIMUM_NODES]);

    static double
    geomListMultiResolutionVisibility(
        const java::ArrayList<Geometry *> *geometryOccluderList,
        ShadowCache *shadowCache,
        Ray *ray,
        float rcvDist,
        float srcSize,
        float minimumFeatureSize);

    static double
    geometryMultiResolutionVisibility(
        ShadowCache *shadowCache,
        Geometry *geometry,
        Ray *ray,
        float rcvDist,
        float srcSize,
        float minimumFeatureSize);
};

#endif

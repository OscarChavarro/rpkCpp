#ifndef __FORM_FACTOR_CLUSTERED_STRATEGY__
#define __FORM_FACTOR_CLUSTERED_STRATEGY__

#include "common/linealAlgebra/Ray.h"
#include "skin/PatchSet.h"
#include "numericalAnalysis/CubatureRule.h"
#include "galerkin/Interaction.h"
#include "galerkin/ShadowCache.h"

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
        const java::ArrayList<PatchSet *> *geometryOccluderList,
        ShadowCache *shadowCache,
        Ray *ray,
        float rcvDist,
        float srcSize,
        float minimumFeatureSize);

    static double
    geometryMultiResolutionVisibility(
        ShadowCache *shadowCache,
        PatchSet *geometry,
        Ray *ray,
        float rcvDist,
        float srcSize,
        float minimumFeatureSize);
};

#endif

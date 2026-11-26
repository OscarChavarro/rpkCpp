#ifndef __FORM_FACTOR_CLUSTERED_STRATEGY__
#define __FORM_FACTOR_CLUSTERED_STRATEGY__

#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
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
        const java::ArrayList<RpkGeometry *> *geometryOccluderList,
        ShadowCache *shadowCache,
        Ray *ray,
        double rcvDist,
        double srcSize,
        double minimumFeatureSize,
        const RayExclusionSet &exclusions);

    static double
    geometryMultiResolutionVisibility(
        ShadowCache *shadowCache,
        RpkGeometry *geometry,
        Ray *ray,
        double rcvDist,
        double srcSize,
        double minimumFeatureSize,
        const RayExclusionSet &exclusions);
};

#endif

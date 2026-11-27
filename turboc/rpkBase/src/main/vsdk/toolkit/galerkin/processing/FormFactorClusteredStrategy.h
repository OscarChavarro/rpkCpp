#ifndef FORM_FCTR_CLSTR_STRTG
#define FORM_FCTR_CLSTR_STRTG

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
        double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES]);

    static double
    geomListMultiResVis(
        const ArrayList<RpkGeometry *> *geometryOccluderList,
        ShadowCache *shadowCache,
        Ray *ray,
        double rcvDist,
        double srcSize,
        double minimumFeatureSize,
        const RayExclusionSet &exclusions);

    static double
    geomMultiResVis(
        ShadowCache *shadowCache,
        RpkGeometry *geometry,
        Ray *ray,
        double rcvDist,
        double srcSize,
        double minimumFeatureSize,
        const RayExclusionSet &exclusions);
};

#endif

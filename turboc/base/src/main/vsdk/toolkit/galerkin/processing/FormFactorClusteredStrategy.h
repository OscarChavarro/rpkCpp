#ifndef FORM_FCTR_CLSTR_STRTG
#define FORM_FCTR_CLSTR_STRTG

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
        double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES]);

    static double
    geomListMultiResVis(
        const ArrayList<Geometry *> *geometryOccluderList,
        ShadowCache *shadowCache,
        Ray *ray,
        float rcvDist,
        float srcSize,
        float minimumFeatureSize,
        const RayExclusionSet &exclusions);

    static double
    geomMultiResVis(
        ShadowCache *shadowCache,
        Geometry *geometry,
        Ray *ray,
        float rcvDist,
        float srcSize,
        float minimumFeatureSize,
        const RayExclusionSet &exclusions);
};

#endif

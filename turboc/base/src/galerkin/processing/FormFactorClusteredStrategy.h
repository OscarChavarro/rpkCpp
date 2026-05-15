#ifndef FORM_FCTR_CLSTR_STRTG
#define FORM_FCTR_CLSTR_STRTG

#include "common/linealAlgebra/Ray.h"
#include "skin/Geometry.h"
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
        double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES]);

    static double
    geomListMultiResVis(
        const ArrayList<Geometry *> *geometryOccluderList,
        ShadowCache *shadowCache,
        Ray *ray,
        float rcvDist,
        float srcSize,
        float minimumFeatureSize);

    static double
    geomMultiResVis(
        ShadowCache *shadowCache,
        Geometry *geometry,
        Ray *ray,
        float rcvDist,
        float srcSize,
        float minimumFeatureSize);
};

#endif

#ifndef __FORM_FACTOR_CLUSTERED_STRATEGY__
#define __FORM_FACTOR_CLUSTERED_STRATEGY__

#include "skin/Geometry.h"

class FormFactorClusteredStrategy {
  public:
    static void
    doConstantAreaToAreaFormFactor(
        Interaction *link,
        const CubatureRule *cubatureRuleRcv,
        const CubatureRule *cubatureRuleSrc,
        double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES]);

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

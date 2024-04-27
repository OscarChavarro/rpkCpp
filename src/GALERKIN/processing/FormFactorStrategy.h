/**
All kind of form factor computations
*/

#ifndef __FORM_FACTOR_STRATEGY__
#define __FORM_FACTOR_STRATEGY__

#include "java/util/ArrayList.h"
#include "scene/VoxelGrid.h"
#include "skin/Geometry.h"
#include "GALERKIN/Interaction.h"
#include "GALERKIN/GalerkinRole.h"

class FormFactorStrategy {
  private:
    static void
    determineNodes(
        GalerkinElement *element,
        CubatureRule **cr,
        Vector3D x[CUBATURE_MAXIMUM_NODES],
        GalerkinRole role,
        const GalerkinState *galerkinState);

    static double
    pointKernelEval(
        const VoxelGrid *sceneWorldVoxelGrid,
        const Vector3D *x,
        const Vector3D *y,
        const GalerkinElement *receiverElement,
        const GalerkinElement *sourceElement,
        const java::ArrayList<Geometry *> *shadowGeometryList,
        double *vis,
        bool isSceneGeometry,
        bool isClusteredGeometry,
        const GalerkinState *galerkinState);

    static void
    doHigherOrderAreaToAreaFormFactor(
        Interaction *link,
        const CubatureRule *cubatureRuleRcv,
        const CubatureRule *cubatureRuleSrc,
        const double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES],
        const GalerkinState *galerkinState);

    static void
    doConstantAreaToAreaFormFactor(
        Interaction *link,
        const CubatureRule *cubatureRuleRcv,
        const CubatureRule *cubatureRuleSrc,
        double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES]);

    static double
    geometryMultiResolutionVisibility(
        Geometry *geometry,
        Ray *ray,
        float rcvDist,
        float srcSize,
        float minimumFeatureSize);

    static double
    geomListMultiResolutionVisibility(
        const java::ArrayList<Geometry *> *geometryOccluderList,
        Ray *ray,
        float rcvDist,
        float srcSize,
        float minimumFeatureSize);

  public:
    static void
    computeAreaToAreaFormFactorVisibility(
        const VoxelGrid *sceneWorldVoxelGrid,
        const java::ArrayList<Geometry *> *geometryShadowList,
        bool isSceneGeometry,
        bool isClusteredGeometry,
        Interaction *link,
        GalerkinState *galerkinState);
};

#endif

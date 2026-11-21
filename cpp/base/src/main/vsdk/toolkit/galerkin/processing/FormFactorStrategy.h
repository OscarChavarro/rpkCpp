/**
All kind of form factor computations
*/

#ifndef FORM_FACTOR_STRATEGY__
#define FORM_FACTOR_STRATEGY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/galerkin/GalerkinBasis.h"
#include "vsdk/toolkit/galerkin/GalerkinRole.h"
#include "vsdk/toolkit/galerkin/Interaction.h"
#include "vsdk/toolkit/galerkin/ShadowCache.h"
#include "vsdk/toolkit/galerkin/processing/FormFactorWorkspace.h"

class FormFactorStrategy {
  private:
    static RpkRayHit *
    shadowTestDiscretization(
        RpkRay *ray,
        const java::ArrayList<RpkGeometry *> *geometrySceneList,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneQueryWorkspace,
        ShadowCache *shadowCache,
        float minimumDistance,
        RpkRayHit *hitStore,
        bool isSceneGeometry,
        bool isClusteredGeometry,
        const RayExclusionSet &exclusions);

    static void
    determineNodes(
        const GalerkinElement *element,
        GalerkinRole role,
        const GalerkinState *galerkinState,
        CubatureRule **cr,
        Vector3D x[CubatureRule::MAXIMUM_NODES]);

    static double
    evaluatePointsPairKernel(
        ShadowCache *shadowCache,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneQueryWorkspace,
        const Vector3D *x,
        const Vector3D *y,
        const GalerkinElement *receiverElement,
        const GalerkinElement *sourceElement,
        const java::ArrayList<RpkGeometry *> *shadowGeometryList,
        bool isSceneGeometry,
        bool isClusteredGeometry,
        const GalerkinState *galerkinState,
        const RayExclusionSet &exclusions);

    static void
    doHigherOrderAreaToAreaFormFactor(
        Interaction *twoPatchesInteraction,
        const CubatureRule *receiverCubatureRule,
        const CubatureRule *sourceCubatureRule,
        const double Gxy[CubatureRule::MAXIMUM_NODES][CubatureRule::MAXIMUM_NODES],
        const GalerkinState *galerkinState);

    static inline void
    computeInteractionError(
        const CubatureRule *receiverCubatureRule,
        const GalerkinElement *receiverElement,
        double gMin,
        double gMax,
        const ColorRgbMutable *sourceRadiance,
        ColorRgbMutable *deltaRadiance,
        Interaction *link);

    static inline void
    computeInteractionFormFactor(
        const CubatureRule *receiverCubatureRule,
        const CubatureRule *sourceCubatureRule,
        const double Gxy[CubatureRule::MAXIMUM_NODES][CubatureRule::MAXIMUM_NODES],
        const GalerkinElement *sourceElement,
        const GalerkinElement *receiverElement,
        const GalerkinBasis *sourceBasis,
        const GalerkinBasis *receiverBasis,
        const ColorRgbMutable *sourceRadiance,
        double *gMin,
        double *gMax,
        ColorRgbMutable *deltaRadiance,
        Interaction *twoPatchesInteraction);

  public:
    static void
    computeAreaToAreaFormFactorVisibility(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneQueryWorkspace,
        FormFactorWorkspace *formFactorWorkspace,
        const java::ArrayList<RpkGeometry *> *geometryShadowList,
        bool isSceneGeometry,
        bool isClusteredGeometry,
        Interaction *link,
        const GalerkinState *galerkinState);
};

#endif

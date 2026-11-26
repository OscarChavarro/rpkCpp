/**
All kind of form factor computations
*/

#ifndef __FORM_FACTOR_STRATEGY__
#define __FORM_FACTOR_STRATEGY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
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
        Ray *ray,
        const java::ArrayList<RpkGeometry *> *geometrySceneList,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneQueryWorkspace,
        FormFactorWorkspace *formFactorWorkspace,
        ShadowCache *shadowCache,
        double minimumDistance,
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
        Vector3Dd x[CubatureRule::MAXIMUM_NODES]);

    static double
    evaluatePointsPairKernel(
        ShadowCache *shadowCache,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneQueryWorkspace,
        FormFactorWorkspace *formFactorWorkspace,
        const Vector3Dd *x,
        const Vector3Dd *y,
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
        const ColorRgb *sourceRadiance,
        double deltaRadiance[][3],
        Interaction *link);

    static void
    boundPointToAreaFormFactors(
        const CubatureRule *receiverCubatureRule,
        const CubatureRule *sourceCubatureRule,
        double sourceArea,
        double Gxy[CubatureRule::MAXIMUM_NODES][CubatureRule::MAXIMUM_NODES]);

    static inline void
    computeInteractionFormFactor(
        const CubatureRule *receiverCubatureRule,
        const CubatureRule *sourceCubatureRule,
        const double Gxy[CubatureRule::MAXIMUM_NODES][CubatureRule::MAXIMUM_NODES],
        const GalerkinElement *sourceElement,
        const GalerkinElement *receiverElement,
        const GalerkinBasis *sourceBasis,
        const GalerkinBasis *receiverBasis,
        const ColorRgb *sourceRadiance,
        double *gMin,
        double *gMax,
        double deltaRadiance[][3],
        Interaction *twoPatchesInteraction);

  public:
    /**
    Warns that two cubature nodes of an element pair are too close to interact
    */
    static void
    reportNodesTooClose(const GalerkinElement *receiverElement, const GalerkinElement *sourceElement);

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

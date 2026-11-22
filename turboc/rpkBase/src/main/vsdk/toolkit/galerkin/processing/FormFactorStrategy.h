/**
All kind of form factor computations
*/

#ifndef __FORM_FACTOR_STRATEGY__
#define __FORM_FACTOR_STRATEGY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/galerkin/GalerkinBasis.h"
#include "vsdk/toolkit/galerkin/GalerkinRole.h"
#include "vsdk/toolkit/galerkin/Interaction.h"
#include "vsdk/toolkit/galerkin/ShadowCache.h"
#include "vsdk/toolkit/galerkin/processing/FormFactorWorkspace.h"

class FormFactorStrategy {
  private:
    static RayHit *
    shadowTestDiscretization(
        RpkRay *ray,
        const ArrayList<Geometry *> *geometrySceneList,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneQueryWorkspace,
        FormFactorWorkspace *formFactorWorkspace,
        ShadowCache *shadowCache,
        float minimumDistance,
        RayHit *hitStore,
        bool isSceneGeometry,
        bool isClusteredGeometry,
        const RayExclusionSet &exclusions);

    static void
    determineNodes(
        const GalerkinElement *element,
        GalerkinRole role,
        const GalerkinState *galerkinState,
        CubatureRule **cr,
        Vector3Df x[CUBATURE_MAXIMUM_NODES]);

    static double
    evaluatePointsPairKernel(
        ShadowCache *shadowCache,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneQueryWorkspace,
        FormFactorWorkspace *formFactorWorkspace,
        const Vector3Df *x,
        const Vector3Df *y,
        const GalerkinElement *receiverElement,
        const GalerkinElement *sourceElement,
        const ArrayList<Geometry *> *shadowGeometryList,
        bool isSceneGeometry,
        bool isClusteredGeometry,
        const GalerkinState *galerkinState,
        const RayExclusionSet &exclusions);

    static void
    dHighOrdAreaTAreaFormFactor(
        Interaction *twoPatchesInteraction,
        const CubatureRule *receiverCubatureRule,
        const CubatureRule *sourceCubatureRule,
        const double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES],
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
        const double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES],
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
    /**
    Warns that two cubature nodes of an element pair are too close to interact
    */
    static void
    reportNodesTooClose(const GalerkinElement *receiverElement, const GalerkinElement *sourceElement);

    static void
    cmptAreaTAreaFormFactorVis(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneQueryWorkspace,
        FormFactorWorkspace *formFactorWorkspace,
        const ArrayList<Geometry *> *geometryShadowList,
        bool isSceneGeometry,
        bool isClusteredGeometry,
        Interaction *link,
        const GalerkinState *galerkinState);
};

#endif

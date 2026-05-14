/**
All kind of form factor computations
*/

#ifndef FORM_FACTOR_STRATEGY__
#define FORM_FACTOR_STRATEGY__

#include "vsdk/toolkit/java/util/ArrayList.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
#include "vsdk/toolkit/galerkin/GalerkinBasis.h"
#include "vsdk/toolkit/galerkin/GalerkinRole.h"
#include "vsdk/toolkit/galerkin/Interaction.h"
#include "vsdk/toolkit/galerkin/ShadowCache.h"

class FormFactorStrategy {
  private:
    // Global variables used for form factor computation optimisation
    static GalerkinElement *formFactorLastReceived;
    static GalerkinElement *formFactorLastSource;

    static RayHit *
    shadowTestDiscretization(
        Ray *ray,
        const java::ArrayList<Geometry *> *geometrySceneList,
        const VoxelGrid *voxelGrid,
        ShadowCache *shadowCache,
        float minimumDistance,
        RayHit *hitStore,
        bool isSceneGeometry,
        bool isClusteredGeometry);

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
        const VoxelGrid *sceneWorldVoxelGrid,
        const Vector3D *x,
        const Vector3D *y,
        const GalerkinElement *receiverElement,
        const GalerkinElement *sourceElement,
        const java::ArrayList<Geometry *> *shadowGeometryList,
        bool isSceneGeometry,
        bool isClusteredGeometry,
        const GalerkinState *galerkinState);

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
        const VoxelGrid *sceneWorldVoxelGrid,
        const java::ArrayList<Geometry *> *geometryShadowList,
        bool isSceneGeometry,
        bool isClusteredGeometry,
        Interaction *link,
        const GalerkinState *galerkinState);
};

#endif

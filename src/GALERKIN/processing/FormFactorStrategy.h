/**
All kind of form factor computations
*/

#ifndef __FORM_FACTOR_STRATEGY__
#define __FORM_FACTOR_STRATEGY__

#include "java/util/ArrayList.h"
#include "scene/VoxelGrid.h"
#include "skin/Geometry.h"
#include "GALERKIN/ShadowCache.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/Interaction.h"
#include "GALERKIN/GalerkinRole.h"

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
        Vector3D x[CUBATURE_MAXIMUM_NODES]);

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
        const double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES],
        const GalerkinState *galerkinState);

    static inline void
    computeInteractionError(
        const CubatureRule *receiverCubatureRule,
        const GalerkinElement *receiverElement,
        double gMin,
        double gMax,
        const ColorRgb *sourceRadiance,
        ColorRgb *deltaRadiance,
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
        const ColorRgb *sourceRadiance,
        double *gMin,
        double *gMax,
        ColorRgb *deltaRadiance,
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

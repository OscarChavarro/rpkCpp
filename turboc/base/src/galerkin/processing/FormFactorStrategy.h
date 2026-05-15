/**
All kind of form factor computations
*/

#ifndef __FORM_FACTOR_STRATEGY__
#define __FORM_FACTOR_STRATEGY__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"
#include "scene/VoxelGrid.h"
#include "galerkin/GalerkinBasis.h"
#include "galerkin/GalerkinRole.h"
#include "galerkin/Interaction.h"
#include "galerkin/ShadowCache.h"

class FormFactorStrategy {
  private:
    // Global variables used for form factor computation optimisation
    static GalerkinElement *formFactorLastReceived;
    static GalerkinElement *formFactorLastSource;

    static RayHit *
    shadowTestDiscretization(
        Ray *ray,
        const ArrayList<Geometry *> *geometrySceneList,
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
        const ArrayList<Geometry *> *shadowGeometryList,
        bool isSceneGeometry,
        bool isClusteredGeometry,
        const GalerkinState *galerkinState);

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
    static void
    cmptAreaTAreaFormFactorVis(
        const VoxelGrid *sceneWorldVoxelGrid,
        const ArrayList<Geometry *> *geometryShadowList,
        bool isSceneGeometry,
        bool isClusteredGeometry,
        Interaction *link,
        const GalerkinState *galerkinState);
};

#endif

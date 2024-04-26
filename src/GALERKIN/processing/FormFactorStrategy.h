/**
All kind of form factor computations
*/

#ifndef __FORM_FACTOR_STRATEGY__
#define __FORM_FACTOR_STRATEGY__

#include "java/util/ArrayList.h"
#include "scene/VoxelGrid.h"
#include "GALERKIN/Interaction.h"
#include "GALERKIN/GalerkinRole.h"

class FormFactorStrategy {
  private:
    static void
    determineNodes(
        GalerkinElement *element,
        CubatureRule **cr,
        Vector3D x[CUBAMAXNODES],
        GalerkinRole role,
        const GalerkinState *galerkinState);

    static double
    pointKernelEval(
        const VoxelGrid *sceneWorldVoxelGrid,
        Vector3D *x,
        Vector3D *y,
        GalerkinElement *receiverElement,
        GalerkinElement *sourceElement,
        const java::ArrayList<Geometry *> *shadowGeometryList,
        double *vis,
        bool isSceneGeometry,
        bool isClusteredGeometry,
        GalerkinState *galerkinState);

    static void
    doHigherOrderAreaToAreaFormFactor(
        Interaction *link,
        CubatureRule *cubatureRuleRcv,
        CubatureRule *cubatureRuleSrc,
        double Gxy[CUBAMAXNODES][CUBAMAXNODES],
        GalerkinState *galerkinState);

    static void
    doConstantAreaToAreaFormFactor(
        Interaction *link,
        CubatureRule *crRcv,
        CubatureRule *crSrc,
        double Gxy[CUBAMAXNODES][CUBAMAXNODES]);

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

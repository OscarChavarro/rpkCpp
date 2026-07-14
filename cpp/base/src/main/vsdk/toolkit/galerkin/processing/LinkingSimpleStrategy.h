#ifndef LINKING_SIMPLE_STRATEGY__
#define LINKING_SIMPLE_STRATEGY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/galerkin/GalerkinRole.h"
#include "vsdk/toolkit/galerkin/GalerkinState.h"

class LinkingSimpleStrategy {
  private:
    static MemoryPool<float> interactionCoefficientsPool;
    static bool interactionCoefficientsPoolInitialized;

    static void ensureInteractionCoefficientsPool();

    static void
    createInitialLink(
        const Scene *scene,
        const GalerkinState *galerkinState,
        GalerkinRole role,
        java::ArrayList<Geometry *> **candidateList,
        GalerkinElement *topElement,
        AxisAlignedBoundingBox *topLevelBoundingBox,
        Patch *patch);

    static void
    geometryLink(
        const Scene *scene,
        const GalerkinState *galerkinState,
        GalerkinRole role,
        java::ArrayList<Geometry *> **candidateList,
        GalerkinElement *topElement,
        AxisAlignedBoundingBox *topLevelBoundingBox,
        Geometry *geometry);
  public:
    static void
    createInitialLinks(
        const Scene *scene,
        const GalerkinState *galerkinState,
        GalerkinRole role,
        GalerkinElement *topElement);
};

#endif

#ifndef __LINKING_CLUSTERED_STRATEGY__
#define __LINKING_CLUSTERED_STRATEGY__

#include "common/memoryManagement/MemoryPool.h"
#include "galerkin/GalerkinElement.h"
#include "galerkin/GalerkinRole.h"

class LinkingClusteredStrategy {
  private:
    static MemoryPool<float> linkingClusteredPool;
    static bool linkingClusteredPoolInitialized;

    static void ensureLinkingClusteredPool();

  public:
    static void
    createInitialLinks(
        GalerkinElement *element,
        GalerkinRole role,
        GalerkinState *galerkinState);
};

#endif

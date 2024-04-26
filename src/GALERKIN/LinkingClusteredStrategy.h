#ifndef __LINKING_CLUSTERED_STRATEGY__
#define __LINKING_CLUSTERED_STRATEGY__

#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/initiallinking.h"

class LinkingClusteredStrategy {
  public:
    static void
    createInitialLinksForTopCluster(
        GalerkinElement *element,
        GalerkinRole role,
        GalerkinState *galerkinState);
};

#endif

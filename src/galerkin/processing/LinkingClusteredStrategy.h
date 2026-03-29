#ifndef __LINKING_CLUSTERED_STRATEGY__
#define __LINKING_CLUSTERED_STRATEGY__

#include "galerkin/GalerkinElement.h"
#include "galerkin/GalerkinRole.h"
class LinkingClusteredStrategy {
  public:
    static void
    createInitialLinks(
        GalerkinElement *element,
        GalerkinRole role,
        GalerkinState *galerkinState);
};

#endif

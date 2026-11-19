#ifndef __LINKING_CLUSTERED_STRATEGY__
#define __LINKING_CLUSTERED_STRATEGY__

#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/galerkin/GalerkinRole.h"

class LinkingClusteredStrategy {
  public:
    static void
    createInitialLinks(
        GalerkinElement *element,
        GalerkinRole role,
        GalerkinState *galerkinState);
};

#endif

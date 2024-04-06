#ifndef __INITIAL_LINKING__
#define __INITIAL_LINKING__

#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/GalerkinState.h"

enum GalerkinRole {
    SOURCE,
    RECEIVER
};

extern void createInitialLinks(GalerkinElement *top, GalerkinRole role, GalerkinState *galerkinState);
extern void createInitialLinkWithTopCluster(GalerkinElement *elem, GalerkinRole role);

#endif

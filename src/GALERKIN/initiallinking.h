#ifndef __INITIAL_LINKING__
#define __INITIAL_LINKING__

#include "GALERKIN/GalerkinElement.h"

enum GalerkinRole {
    SOURCE,
    RECEIVER
};

extern void createInitialLinks(GalerkinElement *top, GalerkinRole role);
extern void createInitialLinkWithTopCluster(GalerkinElement *elem, GalerkinRole role);

#endif

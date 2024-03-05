#ifndef __HIERARCHICAL_REFINE__
#define __HIERARCHICAL_REFINE__

#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/galerkinP.h"

extern void refineInteractions(GalerkinElement *parentElement, GalerkinState *state);

#endif

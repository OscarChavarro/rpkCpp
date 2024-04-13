#ifndef __HIERARCHICAL_REFINE__
#define __HIERARCHICAL_REFINE__

#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/GalerkinState.h"

extern void
refineInteractions(
    GalerkinElement *parentElement,
    GalerkinState *state,
    java::ArrayList<Geometry *> *sceneGeometries,
    Geometry *clusteredWorldGeometry);

#endif

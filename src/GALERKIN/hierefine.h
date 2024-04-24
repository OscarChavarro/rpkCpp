#ifndef __HIERARCHICAL_REFINE__
#define __HIERARCHICAL_REFINE__

#include "scene/VoxelGrid.h"
#include "scene/Scene.h"
#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/GalerkinState.h"

extern void
refineInteractions(
    Scene *scene,
    GalerkinElement *parentElement,
    GalerkinState *state);

#endif

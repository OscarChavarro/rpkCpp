#ifndef __PHOTON_MAP_IMPORTANCE__
#define __PHOTON_MAP_IMPORTANCE__

#include "scene/Background.h"
#include "scene/VoxelGrid.h"

void
tracePotentialPaths(
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    int numberOfPaths);

#endif

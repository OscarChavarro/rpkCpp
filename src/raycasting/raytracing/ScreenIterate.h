/**
Iterates over all pixels of the screen calling
a callback function. This function should return the color for
that pixel. This color is transformed into RGB and displayed.
Several functions are provided for different iterating schemes
*/

#ifndef __SCREEN_ITERATE__
#define __SCREEN_ITERATE__

#include "common/ColorRgb.h"
#include "scene/Background.h"

typedef ColorRgb(*SCREEN_ITERATE_CALLBACK)(Camera *, VoxelGrid *, Background *, int, int, void *);

void
screenIterateSequential(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SCREEN_ITERATE_CALLBACK callback,
    void *data);

void
screenIterateProgressive(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SCREEN_ITERATE_CALLBACK callback,
    void *data);

#endif

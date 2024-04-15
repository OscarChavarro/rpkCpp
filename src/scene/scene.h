#ifndef __SCENE__
#define __SCENE__

#include "java/util/ArrayList.h"
#include "scene/VoxelGrid.h"

// The top of the patch cluster hierarchy for the scene. Automatically derived from scene patches when loading a new scene
extern java::ArrayList<Geometry *> *GLOBAL_scene_clusteredGeometries;

#endif

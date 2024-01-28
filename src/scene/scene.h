#ifndef __SCENE__
#define __SCENE__

#include "java/util/ArrayList.h"
#include "skin/geomlist.h"
#include "skin/PatchSet.h"
#include "scene/Background.h"
#include "scene/VoxelGrid.h"

// The current scene
extern GeometryListNode *GLOBAL_scene_world;

// The list of all materials present in the current scene
extern java::ArrayList<Material *> *GLOBAL_scene_materials;

// The current background (sky, environment map, etc.) for the scene
extern Background *GLOBAL_scene_background;

// The top of the patch cluster hierarchy for the scene. Automatically derived from scene patches when loading a new scene
extern GeometryListNode *GLOBAL_scene_clusteredWorld;

// Single Geometry containing the above
extern Geometry *GLOBAL_scene_clusteredWorldGeom;

// Voxel grid containing the whole world
extern VoxelGrid *GLOBAL_scene_worldVoxelGrid;

#endif

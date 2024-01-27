#ifndef _SCENE_H_
#define _SCENE_H_

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

// The light of all patches on light sources, useful for e.g. next event estimation in Monte Carlo raytracing etc.
extern PatchSet *GLOBAL_scene_lightSourcePatches;

// The top of the patch cluster hierarchy for the scene. Automatically derived from 'GLOBAL_scene_patches' when loading a new scene
extern GeometryListNode *GLOBAL_scene_clusteredWorld;

// Single Geometry containing the above
extern Geometry *GLOBAL_scene_clusteredWorldGeom;

// Voxel grid containing the whole world
extern VoxelGrid *GLOBAL_scene_worldVoxelGrid;

#endif

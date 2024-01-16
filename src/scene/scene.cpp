#include "scene/scene.h"

GeometryListNode *GLOBAL_scene_world = (GeometryListNode *) nullptr;
java::ArrayList<Material *> *GLOBAL_scene_materials = nullptr;
Background *GLOBAL_scene_background = (Background *) nullptr;
PatchSet *GLOBAL_scene_patches = (PatchSet *) nullptr;
PatchSet *GLOBAL_scene_lightSourcePatches = (PatchSet *) nullptr;
GeometryListNode *GLOBAL_scene_clusteredWorld = (GeometryListNode *) nullptr;
Geometry *GLOBAL_scene_clusteredWorldGeom = (Geometry *) nullptr;
VoxelGrid *GLOBAL_scene_worldVoxelGrid = (VoxelGrid *) nullptr;

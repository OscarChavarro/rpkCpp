#include "scene/scene.h"

GeometryListNode *GLOBAL_scene_world = (GeometryListNode *) nullptr;
MATERIALLIST *GLOBAL_scene_materials = (MATERIALLIST *) nullptr;
Background *GLOBAL_scene_background = (Background *) nullptr;
PatchSet *GLOBAL_scene_patches = (PatchSet *) nullptr;
PatchSet *GLOBAL_scene_lightSourcePatches = (PatchSet *) nullptr;
GeometryListNode *GLOBAL_scene_clusteredWorld = (GeometryListNode *) nullptr;
Geometry *GLOBAL_scene_clusteredWorldGeom = (Geometry *) nullptr;
GRID *GLOBAL_scene_worldGrid = (GRID *) nullptr;

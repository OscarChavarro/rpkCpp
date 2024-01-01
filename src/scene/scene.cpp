#include "scene/scene.h"

GEOMLIST *GLOBAL_scene_world = (GEOMLIST *) nullptr;
MATERIALLIST *GLOBAL_scene_materials = (MATERIALLIST *) nullptr;
Background *GLOBAL_scene_background = (Background *) nullptr;
PATCHLIST *GLOBAL_scene_patches = (PATCHLIST *) nullptr;
PATCHLIST *GLOBAL_scene_lightSourcePatches = (PATCHLIST *) nullptr;
GEOMLIST *GLOBAL_scene_clusteredWorld = (GEOMLIST *) nullptr;
Geometry *GLOBAL_scene_clusteredWorldGeom = (Geometry *) nullptr;
GRID *GLOBAL_scene_worldGrid = (GRID *) nullptr;

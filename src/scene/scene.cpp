#include "scene/scene.h"

GeometryListNode *GLOBAL_scene_world = nullptr;
java::ArrayList<Material *> *GLOBAL_scene_materials = nullptr;
Background *GLOBAL_scene_background = nullptr;
GeometryListNode *GLOBAL_scene_clusteredWorld = nullptr;
Geometry *GLOBAL_scene_clusteredWorldGeom = nullptr;
VoxelGrid *GLOBAL_scene_worldVoxelGrid = nullptr;

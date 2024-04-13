#include "scene/scene.h"

Background *GLOBAL_scene_background = nullptr;
java::ArrayList<Geometry *> *GLOBAL_scene_clusteredGeometries = nullptr;
Geometry *GLOBAL_scene_clusteredWorldGeom = nullptr;
VoxelGrid *GLOBAL_scene_worldVoxelGrid = nullptr;

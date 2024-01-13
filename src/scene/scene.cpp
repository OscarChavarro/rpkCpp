#include "java/util/ArrayList.txx"
#include "scene/scene.h"

GeometryListNode *GLOBAL_scene_world = (GeometryListNode *) nullptr;
java::ArrayList<Material *> *GLOBAL_scene_materials = nullptr;
Background *GLOBAL_scene_background = (Background *) nullptr;
java::ArrayList<Patch *> *GLOBAL_scene_patches = new java::ArrayList<Patch *>();
java::ArrayList<Patch *> *GLOBAL_scene_lightSourcePatches = new java::ArrayList<Patch *>();
GeometryListNode *GLOBAL_scene_clusteredWorld = (GeometryListNode *) nullptr;
Geometry *GLOBAL_scene_clusteredWorldGeom = (Geometry *) nullptr;
VoxelGrid *GLOBAL_scene_worldVoxelGrid = (VoxelGrid *) nullptr;

#ifndef __BATCH__
#define __BATCH__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"

// The list of all patches in the current scene. Automatically derived from 'GLOBAL_scene_world' when loading a scene
extern PatchSet *GLOBAL_app_scenePatches;

extern void parseBatchOptions(int *argc, char **argv);
extern void batch(java::ArrayList<Patch *> *scenePatches);

#endif

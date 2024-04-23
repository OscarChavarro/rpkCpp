#ifndef __BATCH__
#define __BATCH__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "scene/Scene.h"

extern void batchParseOptions(int *argc, char **argv);
extern void batchExecuteRadianceSimulation(Scene *scene, RadianceMethod *radianceMethod);

#endif

#ifndef __BATCH__
#define __BATCH__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "scene/Scene.h"

extern void batchExecuteRadianceSimulation(Scene *scene, RadianceMethod *radianceMethod, RenderOptions *renderOptions);
extern void generalParseOptions(int *argc, char **argv);

#endif

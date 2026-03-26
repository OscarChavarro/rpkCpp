#ifndef __BATCH__
#define __BATCH__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "scene/Scene.h"
#include "app/BatchOptions.h"

class RayTracer;

extern void
batchExecuteRadianceSimulation(
    Scene *scene,
    RadianceMethod *radianceMethod,
    const RayTracer *rayTracer,
    RenderOptions *renderOptions);

extern void generalParseOptions(int *argc, char **argv);
extern const BatchOptions *batchGetOptions();

#endif

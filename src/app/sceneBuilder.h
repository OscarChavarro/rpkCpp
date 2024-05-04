#ifndef __SCENE_BUILDER__
#define __SCENE_BUILDER__

#include "io/mgf/MgfContext.h"
#include "scene/Scene.h"

extern void
sceneBuilderCreateModel(
    const int *argc,
    char *const *argv,
    MgfContext *mgfContext,
    Scene *scene);

#endif

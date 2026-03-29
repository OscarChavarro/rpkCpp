#ifndef __SCENE_BUILDER__
#define __SCENE_BUILDER__

#include "scene/Scene.h"
#include "io/context/BaseContext.h"

extern void
sceneBuilderCreateModel(
    const int *argc,
    char *const *argv,
    BaseContext *mgfContext,
    Scene *scene);

#endif

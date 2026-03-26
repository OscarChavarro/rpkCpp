#ifndef __SCENE_BUILDER__
#define __SCENE_BUILDER__

#include "io/context/BaseContext.h"
#include "scene/Scene.h"

extern void
sceneBuilderCreateModel(
    const int *argc,
    char *const *argv,
    BaseContext *mgfContext,
    Scene *scene);

#endif

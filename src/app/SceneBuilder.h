#ifndef __SCENE_BUILDER__
#define __SCENE_BUILDER__

#include "scene/Scene.h"
#include "io/context/MgfParseSession.h"

extern void
sceneBuilderCreateModel(
    const int *argc,
    char *const *argv,
    MgfParseSession *mgfContext,
    Scene *scene);

#endif

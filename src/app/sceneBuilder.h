#ifndef __SCENE_BUILDER__
#define __SCENE_BUILDER__

#include "io/mgf/MgfContext.h"
#include "scene/Scene.h"

extern void
mainBuildModel(
    const int *argc,
    char *const *argv,
    MgfContext *context,
    Scene *scene,
    char **currentWorkingDirectory);

#endif

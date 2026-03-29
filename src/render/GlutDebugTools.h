#ifndef __GLUT__
#define __GLUT__

#include "java/util/ArrayList.h"
#include "scene/RadianceMethod.h"
#include "scene/Scene.h"
#include "io/context/BaseContext.h"
#include "render/GlutDebugState.h"

extern void
executeGlutGui(
    int argc,
    char *argv[],
    Scene *scene,
    RadianceMethod *radianceMethod,
    RenderOptions *renderOptions,
    void (*memoryFreeCallBack)(BaseContext *mgfContext),
    BaseContext *mgfContext);

#endif

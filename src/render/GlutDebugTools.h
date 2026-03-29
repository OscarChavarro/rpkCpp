#ifndef __GLUT__
#define __GLUT__

#include "java/util/ArrayList.h"
#include "scene/RadianceMethod.h"
#include "scene/Scene.h"
#include "io/context/MgfParseSession.h"
#include "render/GlutDebugState.h"

extern void
executeGlutGui(
    int argc,
    char *argv[],
    Scene *scene,
    RadianceMethod *radianceMethod,
    RenderOptions *renderOptions,
    void (*memoryFreeCallBack)(MgfParseSession *mgfContext),
    MgfParseSession *mgfContext);

#endif

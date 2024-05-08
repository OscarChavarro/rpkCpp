#ifndef __GLUT__
#define __GLUT__

#include "java/util/ArrayList.h"
#include "scene/RadianceMethod.h"
#include "scene/Scene.h"

class GlutDebugState {
  public:
    int selectedPatch;
    bool showSelectedPathOnly;
    float angle;

    GlutDebugState();
};

extern GlutDebugState GLOBAL_render_glutDebugState;

extern void
executeGlutGui(int argc, char *argv[], Scene *scene, RadianceMethod *radianceMethod, RenderOptions *renderOptions);

#endif

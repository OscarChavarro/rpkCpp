#ifndef __GLUT__
#define __GLUT__

#include "java/util/ArrayList.h"
#include "skin/RadianceMethod.h"

class GlutDebugState {
  public:
    int selectedPatch;
    bool showSelectedPathOnly;
    float angle;

    GlutDebugState();
};

extern GlutDebugState GLOBAL_render_glutDebugState;

extern void
executeGlutGui(
    int argc,
    char *argv[],
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Patch *> *lightPatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    Geometry *clusteredWorldGeom,
    RadianceMethod *radianceMethod);

#endif
#ifndef __BATCH__
#define __BATCH__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"

extern void parseBatchOptions(int *argc, char **argv);

extern void
batch(
    Background *sceneBackground,
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Patch *> *lightPatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    Geometry *clusteredWorldGeometry,
    RadianceMethod *context);

#endif

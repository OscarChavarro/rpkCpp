#ifndef __BATCH__
#define __BATCH__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"

extern void parseBatchOptions(int *argc, char **argv);

extern void
batch(
    Camera *camera,
    Background *sceneBackground,
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Patch *> *lightPatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    java::ArrayList<Geometry *> *sceneClusteredGeometries,
    Geometry *clusteredWorldGeometry,
    VoxelGrid *voxelGrid,
    RadianceMethod *context);

#endif

#ifndef __RANDOM_WALK_RADIANCE_METHOD__
#define __RANDOM_WALK_RADIANCE_METHOD__

#include "skin/RadianceMethod.h"

class RandomWalkRadianceMethod : public RadianceMethod {
  public:
    RandomWalkRadianceMethod();
    ~RandomWalkRadianceMethod();
    const char *getRadianceMethodName() const;
    void parseOptions(int *argc, char **argv);
    void initialize(java::ArrayList<Patch *> *scenePatches, Geometry *clusteredWorldGeometry);

    int
    doStep(
        Background *sceneBackground,
        java::ArrayList<Patch *> *scenePatches,
        java::ArrayList<Geometry *> *sceneGeometries,
        java::ArrayList<Geometry *> *sceneClusteredGeometries,
        java::ArrayList<Patch *> *lightPatches,
        Geometry *clusteredWorldGeometry,
        VoxelGrid *sceneWorldVoxelGrid);

    void terminate(java::ArrayList<Patch *> *scenePatches);
    ColorRgb getRadiance(Patch *patch, double u, double v, Vector3D dir);
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats();
    void renderScene(java::ArrayList<Patch *> *scenePatches, Geometry *clusteredWorldGeometry);
    void writeVRML(FILE *fp);
};

#endif

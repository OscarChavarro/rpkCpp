#ifndef __RANDOM_WALK_RADIANCE_METHOD__
#define __RANDOM_WALK_RADIANCE_METHOD__

#include "scene/RadianceMethod.h"
#include "scene/Camera.h"

class RandomWalkRadianceMethod : public RadianceMethod {
  public:
    RandomWalkRadianceMethod();
    ~RandomWalkRadianceMethod();
    const char *getRadianceMethodName() const;
    void parseOptions(int *argc, char **argv);
    void initialize(const java::ArrayList<Patch *> *scenePatches, Geometry *clusteredWorldGeometry);

    int
    doStep(
        Camera *camera,
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
    void renderScene(Camera *camera, java::ArrayList<Patch *> *scenePatches, Geometry *clusteredWorldGeometry);
    void writeVRML(Camera *camera, FILE *fp);
};

#endif

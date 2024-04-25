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
    void initialize(Scene *scene);
    int doStep(Scene *scene, RenderOptions *renderOptions);
    void terminate(java::ArrayList<Patch *> *scenePatches);
    ColorRgb getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3D dir);
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats();
    void renderScene(Scene *scene, RenderOptions *renderOptions);
    void writeVRML(Camera *camera, FILE *fp, RenderOptions *renderOptions);
};

#endif

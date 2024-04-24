#ifndef __STOCHASTIC_JACOBI_RADIOSITY_METHOD__
#define __STOCHASTIC_JACOBI_RADIOSITY_METHOD__

#include "scene/RadianceMethod.h"

class StochasticJacobiRadianceMethod : public RadianceMethod {
public:
    StochasticJacobiRadianceMethod();
    ~StochasticJacobiRadianceMethod();
    const char *getRadianceMethodName() const;
    void parseOptions(int *argc, char **argv);
    void initialize(Scene *scene);
    int doStep(Scene *scene, RenderOptions *renderOptions);
    void terminate(java::ArrayList<Patch *> *scenePatches);
    ColorRgb getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3D dir);
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats();
    void renderScene(Scene *scene);
    void writeVRML(Camera *camera, FILE *fp, RenderOptions *renderOptions);
};

#endif

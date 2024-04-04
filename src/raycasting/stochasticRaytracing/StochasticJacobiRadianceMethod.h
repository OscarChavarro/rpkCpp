#ifndef __STOCHASTIC_JACOBI_RADIOSITY_METHOD__
#define __STOCHASTIC_JACOBI_RADIOSITY_METHOD__

#include "skin/RadianceMethod.h"

class StochasticJacobiRadianceMethod : public RadianceMethod {
public:
    StochasticJacobiRadianceMethod();
    ~StochasticJacobiRadianceMethod();
    const char *getRadianceMethodName() const;
    void parseOptions(int *argc, char **argv);
    void initialize(java::ArrayList<Patch *> *scenePatches);
    int doStep(java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches);
    void terminate(java::ArrayList<Patch *> *scenePatches);
    ColorRgb getRadiance(Patch *patch, double u, double v, Vector3D dir);
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats();
    void renderScene(java::ArrayList<Patch *> *scenePatches);
    void writeVRML(FILE *fp);
};

#endif

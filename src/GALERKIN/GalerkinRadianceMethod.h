#ifndef __GALERKIN_RADIOSITY_METHOD__
#define __GALERKIN_RADIOSITY_METHOD__

#include "skin/RadianceMethod.h"

class GalerkinRadianceMethod : public RadianceMethod {
  public:
    GalerkinRadianceMethod();
    ~GalerkinRadianceMethod();
    void defaultValues();
    void parseOptions(int *argc, char **argv);
    void initialize(java::ArrayList<Patch *> *scenePatches);
    int doStep(java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches);
    void terminate(java::ArrayList<Patch *> *scenePatches);
    COLOR getRadiance(Patch *patch, double u, double v, Vector3D dir);
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats();
    void renderScene(java::ArrayList<Patch *> *scenePatches);
    void writeVRML(FILE *fp);
};

extern RADIANCEMETHOD GLOBAL_galerkin_radiosity;

extern void galerkinFreeMemory();

#endif

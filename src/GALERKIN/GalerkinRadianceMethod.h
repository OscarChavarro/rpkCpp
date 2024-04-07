#ifndef __GALERKIN_RADIOSITY_METHOD__
#define __GALERKIN_RADIOSITY_METHOD__

#include "common/cubature.h"
#include "skin/RadianceMethod.h"
#include "GALERKIN/GalerkinState.h"

class GalerkinRadianceMethod : public RadianceMethod {
  private:
    void patchInit(Patch *patch);

  public:
    static GalerkinState galerkinState;

    GalerkinRadianceMethod();
    ~GalerkinRadianceMethod();
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
    void recomputePatchColor(Patch *patch);
};

extern void galerkinFreeMemory();


#endif

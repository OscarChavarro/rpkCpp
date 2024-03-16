#ifndef __PHOTON_MAP_RADIOSITY_
#define __PHOTON_MAP_RADIOSITY_

#include "java/util/ArrayList.h"
#include "skin/RadianceMethod.h"
#include "raycasting/common/pathnode.h"
#include "PHOTONMAP/photonmap.h"

class PhotonMapRadianceMethod : public RadianceMethod {
  public:
    PhotonMapRadianceMethod();
    ~PhotonMapRadianceMethod();
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

COLOR photonMapGetNodeGRadiance(SimpleRaytracingPathNode *node);
COLOR photonMapGetNodeCRadiance(SimpleRaytracingPathNode *node);

#endif

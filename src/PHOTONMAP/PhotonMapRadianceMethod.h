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
    const char *getRadianceMethodName() const;
    void parseOptions(int *argc, char **argv);
    void initialize(java::ArrayList<Patch *> *scenePatches, Geometry *clusteredWorldGeometry);

    int
    doStep(
        java::ArrayList<Patch *> *scenePatches,
        java::ArrayList<Geometry *> *sceneGeometries,
        java::ArrayList<Patch *> *lightPatches,
        Geometry *clusteredWorldGeometry);

    void terminate(java::ArrayList<Patch *> *scenePatches);
    ColorRgb getRadiance(Patch *patch, double u, double v, Vector3D dir);
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats();
    void renderScene(java::ArrayList<Patch *> *scenePatches);
    void writeVRML(FILE *fp);
};

ColorRgb photonMapGetNodeGRadiance(SimpleRaytracingPathNode *node);
ColorRgb photonMapGetNodeCRadiance(SimpleRaytracingPathNode *node);

#endif

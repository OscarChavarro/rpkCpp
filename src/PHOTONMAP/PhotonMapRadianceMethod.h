#ifndef __PHOTON_MAP_RADIOSITY_
#define __PHOTON_MAP_RADIOSITY_

#include "java/util/ArrayList.h"
#include "scene/RadianceMethod.h"
#include "raycasting/common/pathnode.h"
#include "PHOTONMAP/photonmap.h"

class PhotonMapRadianceMethod : public RadianceMethod {
  public:
    PhotonMapRadianceMethod();
    ~PhotonMapRadianceMethod();
    const char *getRadianceMethodName() const;
    void parseOptions(int *argc, char **argv);
    void initialize(Scene *scene);
    int doStep(Scene *scene, RenderOptions *renderOptions);
    void terminate(java::ArrayList<Patch *> *scenePatches);
    ColorRgb getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3D dir, RenderOptions *renderOptions);
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats();
    void renderScene(Scene *scene);
    void writeVRML(Camera *camera, FILE *fp);
};

ColorRgb photonMapGetNodeGRadiance(SimpleRaytracingPathNode *node);
ColorRgb photonMapGetNodeCRadiance(SimpleRaytracingPathNode *node);

#endif

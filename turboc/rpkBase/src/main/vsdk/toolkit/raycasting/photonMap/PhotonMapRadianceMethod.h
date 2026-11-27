#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __PHOTON_MAP_RADIANCE_METHOD__
#define __PHOTON_MAP_RADIANCE_METHOD__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapConfig.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapState.h"
#include "vsdk/toolkit/raycasting/raytracing/SurfaceSampler.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/raycasting/common/SimpleRaytracingPathNode.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMap.h"

class PhotonMapRadianceMethod: public RadianceMethod{ private:
    static const int PHOTON_MAP_STRING_LENGTH = 1000;
    static bool doingLocalRayCasting;

    PhotonMapState &photonMapState;
    PhotonMapConfig &photonMapConfig;

    static void appendStatsText(char *buffer, int *offset, const char *format, ...);
    void photonMapRadiosityUpdateCpuSecs();
    void photonMapChooseSurfaceSampler(SurfaceSampler **samplerPtr);
    ColorRgb
    phtnMapDCompPxlFluxEstmt( Camera *camera, PhotonMapConfig *config, const RadianceMethod *radianceMethod);
    void
    photonMapDoScreenNEE( Camera *camera, const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, PhotonMapConfig *config, const RadianceMethod *radianceMethod);
    bool
    photonMapDoPhotonStore( const Camera *camera, SimpleRaytracingPathNode *node, ColorRgb power);
    void
    photonMapHandlePath( Camera *camera, const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, PhotonMapConfig *config, const RadianceMethod *radianceMethod);
    void
    photonMapTracePath( Camera *camera, const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneSceneQueryWorkspace, Background *sceneBackground, PhotonMapConfig *config, char bsdfFlags);
    void
    photonMapTracePaths( Camera *camera, const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, Background *sceneBackground, PhotonMapConfig *config, int numberOfPaths, char bsdfFlags = BsdfComponentInfo::BSDF_ALL_COMPONENTS, const RadianceMethod *radianceMethod = NULL);
    void
    photonMapBRRealIteration( Camera *camera, const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, Background *sceneBackground, const RadianceMethod *radianceMethod);

  public:
    explicit PhotonMapRadianceMethod(PhotonMapState &photonMapState, PhotonMapConfig &photonMapConfig);
    ~PhotonMapRadianceMethod();
    const char *getRadianceMethodName() const;
    void parseOptions(int *argc, char **argv);
    void initialize(Scene *scene, ToneMappingContext *toneMapOptions);
    bool doStep(Scene *scene, RenderOptions *renderOptions);
    void terminate(ArrayList<Patch *> *scenePatches);
    ColorRgbMutable getRadiance(Camera *camera, const Patch *patch, double u, double v, Vector3Dd dir, const RenderOptions *renderOptions) const;
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats() const;
    void
    writeVRML( const Camera *camera, OutputStream *outputStream, const RenderOptions *renderOptions) const;

    ColorRgb getNodeGRadiance(SimpleRaytracingPathNode *node) const;
    ColorRgb getNodeCRadiance(SimpleRaytracingPathNode *node) const;
};

#endif

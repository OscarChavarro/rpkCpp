#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __PHOTON_MAP_RADIOSITY_
#define __PHOTON_MAP_RADIOSITY_

#include "java/util/ArrayList.h"
#include "raycasting/photonMap/PhotonMapConfig.h"
#include "raycasting/photonMap/PhotonMapState.h"
#include "raycasting/raytracing/SurfaceSampler.h"
#include "scene/RadianceMethod.h"
#include "raycasting/common/SimpleRaytracingPathNode.h"
#include "raycasting/photonMap/PhotonMap.h"

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
    photonMapDoScreenNEE( Camera *camera, const VoxelGrid *sceneWorldVoxelGrid, PhotonMapConfig *config, const RadianceMethod *radianceMethod);
    bool
    photonMapDoPhotonStore( const Camera *camera, SimpleRaytracingPathNode *node, ColorRgb power);
    void
    photonMapHandlePath( Camera *camera, const VoxelGrid *sceneWorldVoxelGrid, PhotonMapConfig *config, const RadianceMethod *radianceMethod);
    void
    photonMapTracePath( Camera *camera, VoxelGrid *sceneVoxelGrid, Background *sceneBackground, PhotonMapConfig *config, char bsdfFlags);
    void
    photonMapTracePaths( Camera *camera, VoxelGrid *sceneWorldVoxelGrid, Background *sceneBackground, PhotonMapConfig *config, int numberOfPaths, char bsdfFlags = BsdfComponentInfo::BSDF_ALL_COMPONENTS, const RadianceMethod *radianceMethod = NULL);
    void
    photonMapBRRealIteration( Camera *camera, VoxelGrid *sceneWorldVoxelGrid, Background *sceneBackground, const RadianceMethod *radianceMethod);

  public:
    explicit PhotonMapRadianceMethod(PhotonMapState &photonMapState, PhotonMapConfig &photonMapConfig);
    ~PhotonMapRadianceMethod();
    const char *getRadianceMethodName() const;
    void parseOptions(int *argc, char **argv);
    void initialize(Scene *scene, ToneMappingContext *toneMapOptions);
    bool doStep(Scene *scene, RenderOptions *renderOptions);
    void terminate(ArrayList<Patch *> *scenePatches);
    ColorRgb getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3D dir, const RenderOptions *renderOptions) const;
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats() const;
    void
    writeVRML( const Camera *camera, OutputStream *outputStream, const RenderOptions *renderOptions) const;

    ColorRgb getNodeGRadiance(SimpleRaytracingPathNode *node) const;
    ColorRgb getNodeCRadiance(SimpleRaytracingPathNode *node) const;
};

#endif

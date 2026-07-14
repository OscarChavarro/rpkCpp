#ifndef PHOTON_MAP_RADIOSITY_
#define PHOTON_MAP_RADIOSITY_

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapConfig.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapState.h"
#include "vsdk/toolkit/raycasting/raytracing/SurfaceSampler.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/raycasting/common/SimpleRaytracingPathNode.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMap.h"

class PhotonMapRadianceMethod final : public RadianceMethod {
  private:
    static constexpr int STRING_LENGTH = 1000;
    static bool doingLocalRayCasting;

    PhotonMapState &photonMapState;
    PhotonMapConfig &photonMapConfig;

    static void appendStatsText(char *buffer, int *offset, const char *format, ...);
    void photonMapRadiosityUpdateCpuSecs();
    void photonMapChooseSurfaceSampler(SurfaceSampler **samplerPtr);
    ColorRgbMutable
    photonMapDoComputePixelFluxEstimate(
        Camera *camera,
        PhotonMapConfig *config,
        const RadianceMethod *radianceMethod);
    void
    photonMapDoScreenNEE(
        Camera *camera,
        const VoxelGrid *sceneWorldVoxelGrid,
        PhotonMapConfig *config,
        const RadianceMethod *radianceMethod);
    bool
    photonMapDoPhotonStore(
        const Camera *camera,
        SimpleRaytracingPathNode *node,
        ColorRgbMutable power);
    void
    photonMapHandlePath(
        Camera *camera,
        const VoxelGrid *sceneWorldVoxelGrid,
        PhotonMapConfig *config,
        const RadianceMethod *radianceMethod);
    void
    photonMapTracePath(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        PhotonMapConfig *config,
        char bsdfFlags);
    void
    photonMapTracePaths(
        Camera *camera,
        VoxelGrid *sceneWorldVoxelGrid,
        Background *sceneBackground,
        PhotonMapConfig *config,
        int numberOfPaths,
        char bsdfFlags = BsdfComponentInfo::BSDF_ALL_COMPONENTS,
        const RadianceMethod *radianceMethod = nullptr);
    void
    photonMapBRRealIteration(
        Camera *camera,
        VoxelGrid *sceneWorldVoxelGrid,
        Background *sceneBackground,
        const RadianceMethod *radianceMethod);

  public:
    explicit PhotonMapRadianceMethod(PhotonMapState &photonMapState, PhotonMapConfig &photonMapConfig);
    ~PhotonMapRadianceMethod() final;
    const char *getRadianceMethodName() const final;
    void parseOptions(int *argc, char **argv) final;
    void initialize(Scene *scene, ToneMappingContext *toneMapOptions) final;
    bool doStep(Scene *scene, RendererConfiguration *renderOptions) final;
    void terminate(java::ArrayList<Patch *> *scenePatches) final;
    ColorRgbMutable getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3D dir, const RendererConfiguration *renderOptions) const final;
    Element *createPatchData(Patch *patch) final;
    void destroyPatchData(Patch *patch) final;
    char *getStats() const final;
    void
    writeVRML(
        const Camera *camera,
        java::OutputStream *outputStream,
        const RendererConfiguration *renderOptions) const final;

    ColorRgbMutable getNodeGRadiance(SimpleRaytracingPathNode *node) const;
    ColorRgbMutable getNodeCRadiance(SimpleRaytracingPathNode *node) const;
};

#endif

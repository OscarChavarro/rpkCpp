#ifndef __PHOTON_MAP_RADIOSITY_
#define __PHOTON_MAP_RADIOSITY_

#include "java/util/ArrayList.h"
#include "photonMap/PhotonMapConfig.h"
#include "photonMap/PhotonMapState.h"
#include "raycasting/raytracing/SurfaceSampler.h"
#include "scene/RadianceMethod.h"
#include "raycasting/common/SimpleRaytracingPathNode.h"
#include "photonMap/PhotonMap.h"

class PhotonMapRadianceMethod final : public RadianceMethod {
  private:
    static constexpr int STRING_LENGTH = 1000;
    static bool doingLocalRayCasting;

    PhotonMapState &photonMapState;
    PhotonMapConfig &photonMapConfig;

    static void appendStatsText(char *buffer, int *offset, const char *format, ...);
    void photonMapRadiosityUpdateCpuSecs();
    void photonMapChooseSurfaceSampler(SurfaceSampler **samplerPtr);
    ColorRgb
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
        ColorRgb power);
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
    void initialize(Scene *scene) final;
    bool doStep(Scene *scene, RenderOptions *renderOptions) final;
    void terminate(java::ArrayList<Patch *> *scenePatches) final;
    ColorRgb getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3D dir, const RenderOptions *renderOptions) const final;
    Element *createPatchData(Patch *patch) final;
    void destroyPatchData(Patch *patch) final;
    char *getStats() const final;
    void
    writeVRML(
        const Camera *camera,
        java::OutputStream *outputStream,
        const RenderOptions *renderOptions) const final;

    ColorRgb getNodeGRadiance(SimpleRaytracingPathNode *node) const;
    ColorRgb getNodeCRadiance(SimpleRaytracingPathNode *node) const;
};

#endif

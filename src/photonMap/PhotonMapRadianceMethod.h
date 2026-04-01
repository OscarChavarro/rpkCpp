#ifndef __PHOTON_MAP_RADIOSITY_
#define __PHOTON_MAP_RADIOSITY_

#include "java/util/ArrayList.h"
#include "scene/RadianceMethod.h"
#include "raycasting/common/SimpleRaytracingPathNode.h"
#include "photonMap/PhotonMap.h"

class SurfaceSampler;
class PhotonMapConfig;

class PhotonMapRadianceMethod final : public RadianceMethod {
  private:
    static void appendStatsText(char *buffer, int *offset, const char *format, ...);
    static void photonMapRadiosityUpdateCpuSecs();
    static void photonMapChooseSurfaceSampler(SurfaceSampler **samplerPtr);
    static ColorRgb
    photonMapDoComputePixelFluxEstimate(
        Camera *camera,
        PhotonMapConfig *config,
        const RadianceMethod *radianceMethod);
    static void
    photonMapDoScreenNEE(
        Camera *camera,
        const VoxelGrid *sceneWorldVoxelGrid,
        PhotonMapConfig *config,
        const RadianceMethod *radianceMethod);
    static bool
    photonMapDoPhotonStore(
        const Camera *camera,
        SimpleRaytracingPathNode *node,
        ColorRgb power);
    static void
    photonMapHandlePath(
        Camera *camera,
        const VoxelGrid *sceneWorldVoxelGrid,
        PhotonMapConfig *config,
        const RadianceMethod *radianceMethod);
    static void
    photonMapTracePath(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        PhotonMapConfig *config,
        char bsdfFlags);
    static void
    photonMapTracePaths(
        Camera *camera,
        VoxelGrid *sceneWorldVoxelGrid,
        Background *sceneBackground,
        int numberOfPaths,
        char bsdfFlags = BSDF_ALL_COMPONENTS,
        const RadianceMethod *radianceMethod = nullptr);
    static void
    photonMapBRRealIteration(
        Camera *camera,
        VoxelGrid *sceneWorldVoxelGrid,
        Background *sceneBackground,
        const RadianceMethod *radianceMethod);

  public:
    PhotonMapRadianceMethod();
    ~PhotonMapRadianceMethod() final;
    const char *getRadianceMethodName() const final;
    void parseOptions(int *argc, char **argv) final;
    void initialize(Scene *scene) final;
    bool doStep(Scene *scene, RenderOptions *renderOptions) final;
    void terminate(java::ArrayList<Patch *> *scenePatches) final;
    ColorRgb getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3D dir, const RenderOptions *renderOptions) const final;
    Element *createPatchData(Patch *patch) final;
    void destroyPatchData(Patch *patch) final;
    char *getStats() final;
    void
    writeVRML(
        const Camera *camera,
        java::OutputStream *outputStream,
        const RenderOptions *renderOptions) const final;

    static ColorRgb getNodeGRadiance(SimpleRaytracingPathNode *node);
    static ColorRgb getNodeCRadiance(SimpleRaytracingPathNode *node);
};

#endif

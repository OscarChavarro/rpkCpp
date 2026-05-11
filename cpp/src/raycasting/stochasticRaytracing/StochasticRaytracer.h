#ifndef STOCHASTIC_RAYTRACER__
#define STOCHASTIC_RAYTRACER__

#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "raycasting/common/RayTracer.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingConfiguration.h"

class StochasticRaytracer final : public RayTracer {
  private:
    static constexpr float PHOTON_MAP_MIN_DIST = 0.02F;
    static constexpr float PHOTON_MAP_MIN_DIST2 = PHOTON_MAP_MIN_DIST * PHOTON_MAP_MIN_DIST;
    static char name[];
    LightList *&lightList;
    StochasticRayTracingState &rayTracingState;

    static ColorRgb
    calcPixel(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        int nx,
        int ny,
        void *data);
    static ColorRgb stochasticRaytracerGetScatteredRadiance(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *thisNode,
        StochasticRaytracingConfiguration *config,
        StorageReadout readout,
        RadianceMethod *radianceMethod,
        RendererConfiguration *renderOptions);
    static ColorRgb srGetDirectRadiance(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *prevNode,
        StochasticRaytracingConfiguration *config,
        StorageReadout readout);
    static ColorRgb stochasticRaytracerGetRadiance(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *thisNode,
        StochasticRaytracingConfiguration *config,
        StorageReadout readout,
        int usedScatterSamples,
        RadianceMethod *radianceMethod,
        RendererConfiguration *renderOptions);

  public:
    StochasticRaytracer(
        LightList *&inLightList,
        StochasticRayTracingState &inRayTracingState);
    ~StochasticRaytracer() final;

    void defaults() final;
    const char *getName() const final;
    void initialize(const java::ArrayList<Patch *> *lightPatches) const final;

    void
    execute(
        ImageOutputHandle *ip,
        Scene *scene,
        RadianceMethod *radianceMethod,
        ToneMappingContext *toneMapOptions,
        const RendererConfiguration *renderOptions) const final;

    bool saveImage(ImageOutputHandle *imageOutputHandle) const final;
    void terminate() const final;
};

#endif
#endif

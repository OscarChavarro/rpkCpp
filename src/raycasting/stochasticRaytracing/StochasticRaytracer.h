#ifndef __STOCHASTIC_RAYTRACER__
#define __STOCHASTIC_RAYTRACER__

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/common/RayTracer.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingConfiguration.h"

class StochasticRaytracer final : public RayTracer {
  private:
    static char name[];

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
        RenderOptions *renderOptions);
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
        RenderOptions *renderOptions);

  public:
    StochasticRaytracer();
    ~StochasticRaytracer() final;

    void defaults() final;
    const char *getName() const final;
    void initialize(const java::ArrayList<Patch *> *lightPatches) const final;

    void
    execute(
        ImageOutputHandle *ip,
        Scene *scene,
        RadianceMethod *radianceMethod,
        const RenderOptions *renderOptions) const final;

    bool saveImage(ImageOutputHandle *imageOutputHandle) const final;
    void terminate() const final;
};

#endif
#endif

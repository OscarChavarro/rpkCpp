#ifndef __BI_DIRECTIONAL_PATH__
#define __BI_DIRECTIONAL_PATH__

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/common/Raytracer.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingConfiguration.h"

class BidirectionalPathRaytracer final : public RayTracer {
  private:
    static char name[];

    static void
    doBptAndSubsequentImages(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        BidirectionalPathTracingConfiguration *config);

    static void
    doBptDensityEstimation(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        BidirectionalPathTracingConfiguration *config);

    static ColorRgb
    bpCalcPixel(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        int nx,
        int ny,
        BidirectionalPathTracingConfiguration *config);

  public:
    BidirectionalPathRaytracer();
    ~BidirectionalPathRaytracer() final;

    void defaults() final;
    const char *getName() const final;
    void initialize(const java::ArrayList<Patch *> *lightPatches) const final;

    void
    execute(
        ImageOutputHandle *ip,
        Scene *scene,
        RadianceMethod *radianceMethod,
        const RenderOptions *renderOptions) const final;

    bool reDisplay() const final;
};

extern Raytracer GLOBAL_raytracing_biDirectionalPathMethod;

#endif
#endif

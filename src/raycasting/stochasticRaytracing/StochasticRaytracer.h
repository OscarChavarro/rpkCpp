#ifndef __STOCHASTIC_RAYTRACER__
#define __STOCHASTIC_RAYTRACER__

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED


#include "raycasting/common/Raytracer.h"
#include "raycasting/stochasticRaytracing/rtstochasticphotonmap.h"

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
        StochasticRaytracingConfiguration *config,
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
};

extern Raytracer GLOBAL_raytracing_stochasticMethod;

#endif
#endif

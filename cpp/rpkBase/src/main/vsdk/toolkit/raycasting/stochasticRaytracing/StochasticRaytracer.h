#ifndef __STOCHASTIC_RAYTRACER__
#define __STOCHASTIC_RAYTRACER__

#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LightList.h"
#include "vsdk/toolkit/raycasting/common/RayTracer.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRaytracingConfiguration.h"

class StochasticRaytracerCallbackData;

class StochasticRaytracer final : public RayTracer {
  private:
    class FinalTraceWorker;

    void traceRandomWalkInParallel(Scene *scene, RadianceMethod *radianceMethod,
        const RendererConfiguration *renderOptions, StochasticRaytracingConfiguration &config) const;

    static constexpr double PHOTON_MAP_MIN_DIST = 0.02;
    static constexpr double PHOTON_MAP_MIN_DIST2 = PHOTON_MAP_MIN_DIST * PHOTON_MAP_MIN_DIST;
    static char name[];
    LightList *&lightList;
    StochasticRayTracingState &rayTracingState;

    static ColorRgb
    calcPixel(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneSceneQueryWorkspace,
        Background *sceneBackground,
        int nx,
        int ny,
        StochasticRaytracerCallbackData *data);
    static ColorRgb stochasticRaytracerGetScatteredRadiance(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneSceneQueryWorkspace,
        Background *sceneBackground,
        SimpleRaytracingPathNode *thisNode,
        StochasticRaytracingConfiguration *config,
        StorageReadout readout,
        RadianceMethod *radianceMethod,
        RendererConfiguration *renderOptions);
    static ColorRgb srGetDirectRadiance(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneSceneQueryWorkspace,
        Background *sceneBackground,
        SimpleRaytracingPathNode *prevNode,
        StochasticRaytracingConfiguration *config,
        StorageReadout readout);
    static ColorRgb stochasticRaytracerGetRadiance(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneSceneQueryWorkspace,
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
        ImageOutputTarget *ip,
        Scene *scene,
        RadianceMethod *radianceMethod,
        ToneMappingContext *toneMapOptions,
        const RendererConfiguration *renderOptions) const final;

    bool saveImage(ImageOutputTarget *imageOutputTarget) const final;
    void terminate() const final;
};

#endif
#endif

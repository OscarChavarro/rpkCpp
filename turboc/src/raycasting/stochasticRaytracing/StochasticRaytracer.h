#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __STOCHASTIC_RAYTRACER__
#define __STOCHASTIC_RAYTRACER__

#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "raycasting/common/RayTracer.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingConfiguration.h"

class StochasticRaytracer: public RayTracer{ private:
    #define PHOTON_MAP_MIN_DIST ((float)0.02)
    #define PHOTON_MAP_MIN_DIST2 PHOTON_MAP_MIN_DIST * PHOTON_MAP_MIN_DIST
    static char name[];
    LightList *&lightList;
    StochasticRayTracingState &rayTracingState;

    static ColorRgb
    calcPixel( Camera *camera, VoxelGrid *sceneVoxelGrid, Background *sceneBackground, int nx, int ny, void *data);
    static ColorRgb stchsRaytrcGetScttrRadn( Camera *camera, VoxelGrid *sceneVoxelGrid, Background *sceneBackground, SimpleRaytracingPathNode *thisNode, StochRaytrConfig *config, StorageReadout readout, RadianceMethod *radianceMethod, RenderOptions *renderOptions);
    static ColorRgb srGetDirectRadiance( Camera *camera, VoxelGrid *sceneVoxelGrid, Background *sceneBackground, SimpleRaytracingPathNode *prevNode, StochRaytrConfig *config, StorageReadout readout);
    static ColorRgb stochasticRaytracerGetRadiance( Camera *camera, VoxelGrid *sceneVoxelGrid, Background *sceneBackground, SimpleRaytracingPathNode *thisNode, StochRaytrConfig *config, StorageReadout readout, int usedScatterSamples, RadianceMethod *radianceMethod, RenderOptions *renderOptions);

  public:
    StochasticRaytracer( LightList *&inLightList, StochasticRayTracingState &inRayTracingState);
    ~StochasticRaytracer();

    void defaults();
    const char *getName() const;
    void initialize(const ArrayList<Patch *> *lightPatches) const;

    void
    execute( ImageOutputHandle *ip, Scene *scene, RadianceMethod *radianceMethod, ToneMappingContext *toneMapOptions, const RenderOptions *renderOptions) const;

    bool saveImage(ImageOutputHandle *imageOutputHandle) const;
    void terminate() const;
};

#endif
#endif

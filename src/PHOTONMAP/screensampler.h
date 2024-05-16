/**
Samples a random point on the view screen and traces the viewing ray.
*/

#ifndef __SCREEN_SAMPLER__
#define __SCREEN_SAMPLER__

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "scene/Camera.h"
#include "raycasting/raytracing/sampler.h"

class ScreenSampler final : public Sampler {
  public:
    bool
    sample(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x1,
        double x2,
        bool doRR = false,
        char flags = BSDF_ALL_COMPONENTS) final;

    double
    evalPDF(
        Camera *camera,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        char flags = BSDF_ALL_COMPONENTS,
        double *probabilityDensityFunction = nullptr,
        double *probabilityDensityFunctionRR = nullptr) final;
};

#endif

#endif

/**
Samples a direction from a light source point.
It's kind of a dual of a pixel sampler.
*/

#ifndef __LIGHT_DIR_SAMPLER__
#define __LIGHT_DIR_SAMPLER__

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/raytracing/sampler.h"

class LightDirSampler final : public Sampler {
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

/**
Samples a random point on the view screen and traces the viewing ray.
*/

#ifndef __SCREEN_SAMPLER__
#define __SCREEN_SAMPLER__

#include "raycasting/raytracing/sampler.h"

class ScreenSampler : public Sampler {
public:
    bool
    sample(
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x1,
        double x2,
        bool doRR = false,
        BSDF_FLAGS flags = BSDF_ALL_COMPONENTS);

    virtual double
    evalPDF(
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        BSDF_FLAGS flags = BSDF_ALL_COMPONENTS,
        double *probabilityDensityFunction = nullptr,
        double *probabilityDensityFunctionRR = nullptr);
};

#endif

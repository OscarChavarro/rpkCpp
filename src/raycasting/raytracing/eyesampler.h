/**
Just fills in the eye point in the node
*/

#ifndef _EYE_SAMPLER__
#define _EYE_SAMPLER__

#include "raycasting/raytracing/sampler.h"

class CEyeSampler : public Sampler {
public:
    // Sample : newNode gets filled, others may change
    virtual bool
    sample(
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x_1,
        double x_2,
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

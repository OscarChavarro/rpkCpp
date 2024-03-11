/**
Samples a direction from a light source point.
It's kind of a dual of a pixelsampler.
*/

#ifndef __LIGHT_DIR_SAMPLER__
#define __LIGHT_DIR_SAMPLER__

#include "raycasting/raytracing/sampler.h"

class CLightDirSampler : public Sampler {
public:
    virtual bool
    sample(
            SimpleRaytracingPathNode *prevNode,
            SimpleRaytracingPathNode *thisNode,
            SimpleRaytracingPathNode *newNode,
            double x1,
            double x2,
            bool doRR = false,
            BSDF_FLAGS flags = BSDF_ALL_COMPONENTS);

    virtual double
    EvalPDF(
            SimpleRaytracingPathNode *thisNode,
            SimpleRaytracingPathNode *newNode,
            BSDF_FLAGS flags = BSDF_ALL_COMPONENTS,
            double *pdf = nullptr,
            double *pdfRR = nullptr);
};

#endif

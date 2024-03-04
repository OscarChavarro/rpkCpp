/**
Samples a random point on the viewscreen and traces the viewing ray.
*/

#ifndef __SCREEN_SAMPLER__
#define __SCREEN_SAMPLER__

#include "raycasting/raytracing/sampler.h"

class ScreenSampler : public Sampler {
public:
    bool
    Sample(
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x1,
        double x2,
        bool doRR = false,
        BSDFFLAGS flags = BSDF_ALL_COMPONENTS);

    virtual double
    EvalPDF(
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        BSDFFLAGS flags = BSDF_ALL_COMPONENTS,
        double *pdf = nullptr,
        double *pdfRR = nullptr);
};

#endif

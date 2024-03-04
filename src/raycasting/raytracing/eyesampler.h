/**
Just fills in the eye point in the node
*/

#ifndef _EYESAMPLER_H_
#define _EYESAMPLER_H_

#include "raycasting/raytracing/sampler.h"

class CEyeSampler : public CSampler {
public:
    // Sample : newNode gets filled, others may change
    virtual bool Sample(SimpleRaytracingPathNode *prevNode, SimpleRaytracingPathNode *thisNode,
                        SimpleRaytracingPathNode *newNode, double x_1, double x_2,
                        bool doRR = false,
                        BSDFFLAGS flags = BSDF_ALL_COMPONENTS);

    virtual double EvalPDF(SimpleRaytracingPathNode *thisNode, SimpleRaytracingPathNode *newNode,
                           BSDFFLAGS flags = BSDF_ALL_COMPONENTS,
                           double *pdf = nullptr, double *pdfRR = nullptr);
};

#endif

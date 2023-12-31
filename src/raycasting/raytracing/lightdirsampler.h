/*
 * lightdirsampler.H : Samples a direction from a light source point.
 * It's kind of a dual of a pixelsampler.
 */

#ifndef _LIGHTDIRSAMPLER_H_
#define _LIGHTDIRSAMPLER_H_

#include "raycasting/raytracing/sampler.h"

class CLightDirSampler : public CSampler {
public:
    // Sample : newNode gets filled, others may change
    virtual bool Sample(CPathNode *prevNode, CPathNode *thisNode,
                        CPathNode *newNode, double x_1, double x_2,
                        bool doRR = false,
                        BSDFFLAGS flags = BSDF_ALL_COMPONENTS);

    virtual double EvalPDF(CPathNode *thisNode, CPathNode *newNode,
                           BSDFFLAGS flags = BSDF_ALL_COMPONENTS,
                           double *pdf = nullptr, double *pdfRR = nullptr);
};

#endif

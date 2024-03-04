/* screensampler.H
 *
 * samples a random point on the viewscreen and traces the viewing ray. 
 */

#ifndef _SCREENSAMPLER_H_
#define _SCREENSAMPLER_H_

#include "raycasting/raytracing/sampler.h"

class CScreenSampler : public CSampler {
public:
    // Sample : newNode gets filled, others may change
    virtual bool
    Sample(
            SimpleRaytracingPathNode *prevNode,
            SimpleRaytracingPathNode *thisNode,
            SimpleRaytracingPathNode *newNode,
            double x_1,
            double x_2,
            bool doRR = false,
            BSDFFLAGS flags = BSDF_ALL_COMPONENTS);

    virtual double
    EvalPDF(
            SimpleRaytracingPathNode *thisNode,
            SimpleRaytracingPathNode *newNode,
            BSDFFLAGS flags = BSDF_ALL_COMPONENTS,
            double *pdf = nullptr,
            double *pdfRR = nullptr);

  protected:
    double m_h;
    double m_v;
};

#endif

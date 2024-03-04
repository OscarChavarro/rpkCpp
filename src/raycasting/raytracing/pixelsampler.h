#ifndef __PIXEL_SAMPLER__
#define __PIXEL_SAMPLER__

#include "scene/Camera.h"
#include "raycasting/raytracing/sampler.h"

class CPixelSampler : public Sampler {
public:
    // Sample : newNode gets filled, others may change
    virtual bool
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

    // Set pixel : sets the current pixel. This pixel will be sampled
    void SetPixel(int nx, int ny, Camera *cam = nullptr);

protected:
    double m_px, m_py;
};

#endif

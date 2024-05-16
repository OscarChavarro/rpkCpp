#ifndef __PIXEL_SAMPLER__
#define __PIXEL_SAMPLER__

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "scene/Camera.h"
#include "raycasting/raytracing/sampler.h"

class CPixelSampler : public Sampler {
  private:
    double m_px;
    double m_py;
  public:
    // Sample : newNode gets filled, others may change
    virtual bool
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
        char flags = BSDF_ALL_COMPONENTS);

    virtual double
    evalPDF(
        Camera *camera,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        char flags = BSDF_ALL_COMPONENTS,
        double *pdf = nullptr,
        double *pdfRR = nullptr);

    // Set pixel : sets the current pixel. This pixel will be sampled
    void SetPixel(const Camera *defaultCamera, int nx, int ny, const Camera *camera);
};

#endif

#endif

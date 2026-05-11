#ifndef PIXEL_SAMPLER__
#define PIXEL_SAMPLER__

#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/raycasting/raytracing/Sampler.h"

class PixelSampler : public Sampler {
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
        char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS);

    double
    evalPDF(
        Camera *camera,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS,
        double *pdf = nullptr,
        double *pdfRR = nullptr) final;

    // Set pixel : sets the current pixel. This pixel will be sampled
    void SetPixel(const Camera *defaultCamera, int nx, int ny, const Camera *camera);
};

#endif

#endif

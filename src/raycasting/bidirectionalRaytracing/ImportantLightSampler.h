#ifndef __IMPORTANT_LIGHT_SAMPLER__
#define __IMPORTANT_LIGHT_SAMPLER__

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "raycasting/raytracing/sampler.h"

class ImportantLightSampler final : public CNextEventSampler {
  public:
    // Sample : newNode gets filled, others may change
    bool
    sample(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x1,
        double x2,
        bool doRR,
        char flags) final;

    double
    evalPDF(
        Camera *camera,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        char flags,
        double *pdf,
        double *pdfRR) final;
};

#endif

#endif

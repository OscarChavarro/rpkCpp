#ifndef __UNIFORM_LIGHT_SAMPLER__
#define __UNIFORM_LIGHT_SAMPLER__

#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "raycasting/raytracing/Sampler.h"

class UniformLightSampler final : public NextEventSampler {
  private:
    LightList *lightList;
    LightListIterator *iterator;
    Patch *currentPatch;
    bool unitsActive;
  public:
    explicit UniformLightSampler(LightList *inLightList);
    ~UniformLightSampler() final;

    bool ActivateFirstUnit() final;

    bool ActivateNextUnit() final;

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

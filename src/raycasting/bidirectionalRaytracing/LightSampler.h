/**
Samples a point on a light source. Two implementations are given : uniform sampling and
importance sampling
*/

#ifndef __LIGHT_SAMPLER__
#define __LIGHT_SAMPLER__

#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "raycasting/raytracing/sampler.h"

class UniformLightSampler : public CNextEventSampler {
  private:
    LightListIterator *iterator;
    Patch *currentPatch;
    bool unitsActive;
  public:
    UniformLightSampler();

    virtual bool ActivateFirstUnit();

    virtual bool ActivateNextUnit();

    // Sample : newNode gets filled, others may change
    virtual bool
    sample(
        Background *sceneBackground,
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x1,
        double x2,
        bool doRR,
        BSDF_FLAGS flags);

    virtual double
    evalPDF(
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        BSDF_FLAGS flags,
        double *pdf,
        double *pdfRR);
};

class ImportantLightSampler : public CNextEventSampler {
  public:
    // Sample : newNode gets filled, others may change
    virtual bool
    sample(
        Background *sceneBackground,
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x1,
        double x2,
        bool doRR,
        BSDF_FLAGS flags);

    virtual double
    evalPDF(
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        BSDF_FLAGS flags,
        double *pdf,
        double *pdfRR);
};

#endif

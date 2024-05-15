/**
Path node sampler using bsdf sampling
*/

#ifndef __BSDF_SAMPLER__
#define __BSDF_SAMPLER__

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/raytracing/sampler.h"

class CBsdfSampler : public CSurfaceSampler {
  public:
    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path ends.
    //   If path ends (absorption) the type of thisNode is adjusted to 'Ends'
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
        char flags) override;

    // Use this for N.E.E. : connecting a light node with an eye node
    double
    evalPDF(
        Camera *camera,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        char flags,
        double *pdf = nullptr,
        double *pdfRR = nullptr) override;

    // Use this for calculating f.i. eyeEndNode->Previous pdf(Next).
    // The newNode is calculated, thisNode should be and end node connecting
    // to another sub path end node. prevNode is that other subpath
    // endNode.
    double
    EvalPDFPrev(
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        char flags,
        double *pdf,
        double *pdfRR) override;
};

#endif

#endif

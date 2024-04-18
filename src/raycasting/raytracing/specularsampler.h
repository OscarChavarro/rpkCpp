/**
Path node sampler using PERFECT SPECULAR reflection/refraction only

BEWARE
This is a 'dirac' sampler, meaning that the pdf
for generating the outgoing direction is infinite (dirac)
You CANNOT use these samplers together with other samplers
for multiple importance sampling!

All probabilityDensityFunction evaluations should be multiplied by infinity.

I currently use it only in classical raytracing
*/

#ifndef __SPECULAR_SAMPLER__
#define __SPECULAR_SAMPLER__

#include "raycasting/raytracing/sampler.h"

class CSpecularSampler : public CSurfaceSampler {
public:
    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path Ends
    //   When path ends (absorption) the type of thisNode is adjusted to 'Ends'

    // *** 'flags' is used to determine the amount of energy that is
    // reflected/refracted. If there are both reflective AND refractive
    // components, a scatter mode is chosen randomly !!

    virtual bool
    sample(
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x1,
        double x2,
        bool doRR,
        BSDF_FLAGS flags);

    // Use this for N.E.E. : connecting a light node with an eye node

    // Return value should be multiplied by infinity!
    virtual double
    evalPDF(
        Camera *camera,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        BSDF_FLAGS flags,
        double *probabilityDensityFunction = nullptr,
        double *probabilityDensityFunctionRR = nullptr);

    // Use this for calculating f.i. eyeEndNode->Previous probabilityDensityFunction(Next).
    // The newNode is calculated, thisNode should be and end node connecting
    // to another sub path end node. prevNode is that other sub-path
    // endNode
    // Return value should be multiplied by infinity!
    virtual double
    EvalPDFPrev(
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        BSDF_FLAGS flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR);
};

#endif
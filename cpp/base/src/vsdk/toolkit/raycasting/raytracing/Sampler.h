/**
Generic class for samplers. Samplers operate on
path nodes and have to possible actions :
  - sample : fill in a new path node and evaluate bsdf's and pdf's
             where necessary.
  - connect : connect to sub-paths and evaluate the necessary
              bsdf's and pdf's.
*/

#ifndef SAMPLER__
#define SAMPLER__

#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED
#include "vsdk/toolkit/material/PhongBidirectionalScatteringDistributionFunction.h"
#include "vsdk/toolkit/raycasting/common/SimpleRaytracingPathNode.h"
#include "vsdk/toolkit/scene/Background.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
#include "vsdk/toolkit/scene/Camera.h"

class Sampler {
  protected:
    virtual bool sampleTransfer(
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        Vector3D *dir,
        double pdfDir);

public:
    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path Ends
    //   When path ends (absorption) the type of thisNode is adjusted to 'Ends'
    Sampler();
    virtual ~Sampler();

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
        char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS) = 0;

    virtual double
    evalPDF(
        Camera *camera,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS,
        double *probabilityDensityFunction = nullptr,
        double *probabilityDensityFunctionRR = nullptr) = 0;
};

inline Sampler::Sampler() {
}

inline Sampler::~Sampler() {
}

#include "vsdk/toolkit/raycasting/raytracing/NextEventSampler.h"
#include "vsdk/toolkit/raycasting/raytracing/SurfaceSampler.h"

#endif

#endif

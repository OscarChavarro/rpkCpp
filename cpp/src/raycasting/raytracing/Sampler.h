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

#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "raycasting/common/SimpleRaytracingPathNode.h"
#include "scene/Background.h"
#include "scene/VoxelGrid.h"
#include "scene/Camera.h"

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

#include "raycasting/raytracing/NextEventSampler.h"
#include "raycasting/raytracing/SurfaceSampler.h"

#endif

#endif

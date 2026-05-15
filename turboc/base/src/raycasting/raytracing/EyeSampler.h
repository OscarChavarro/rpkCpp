/**
Just fills in the eye point in the node
*/

#ifndef _EYE_SAMPLER__
#define _EYE_SAMPLER__

#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/raytracing/Sampler.h"

class EyeSampler: public Sampler{ public:
    // Sample: newNode gets filled, others may change
    bool
    sample( Camera *camera, VoxelGrid *sceneVoxelGrid, Background *sceneBackground, SimpleRaytracingPathNode *prevNode, SimpleRaytracingPathNode *thisNode, SimpleRaytracingPathNode *newNode, double x1, double x2, bool doRR = false, char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS);

    double
    evalPDF( Camera *camera, SimpleRaytracingPathNode *thisNode, SimpleRaytracingPathNode *newNode, char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS, double *probabilityDensityFunction = NULL, double *probabilityDensityFunctionRR = NULL);
};

#endif

#endif

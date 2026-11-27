/**
Just fills in the eye point in the node
*/

#ifndef __EYE_SAMPLER__
#define __EYE_SAMPLER__

#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/raycasting/raytracing/Sampler.h"

class EyeSampler: public Sampler{ public:
    // Sample: newNode gets filled, others may change
    bool
    sample( Camera *camera, const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneSceneQueryWorkspace, Background *sceneBackground, SimpleRaytracingPathNode *prevNode, SimpleRaytracingPathNode *thisNode, SimpleRaytracingPathNode *newNode, double x1, double x2, bool doRR = false, char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS);

    double
    evalPDF( Camera *camera, SimpleRaytracingPathNode *thisNode, SimpleRaytracingPathNode *newNode, char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS, double *probabilityDensityFunction = NULL, double *probabilityDensityFunctionRR = NULL);
};

#endif

#endif

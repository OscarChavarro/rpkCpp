/**
A sampler specifically designed for use with photon maps.
Specular materials are treated as Fresnel reflectors/refractors.

NO DIFFUSE OR GLOSSY TRANSMITTING SURFACES SUPPORTED YET!
*/

#ifndef __PHOTON_MAP_SAMPLER__
#define __PHOTON_MAP_SAMPLER__

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/raytracing/bsdfsampler.h"
#include "PHOTONMAP/photonmap.h"

class CPhotonMapSampler final : public CBsdfSampler {
  private:
    CPhotonMap *m_photonMap; // To be used for importance sampling

    bool
    fresnelSample(
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        const SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x2,
        char flags);

    bool
    gdSample(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x1,
        double x2,
        bool doRR,
        char flags);

    // Randomly choose between 2 scattering components, using
    // scattered power as probabilities.
    // Returns true a component was chosen, false if absorbed
    static bool
    chooseComponent(
        char flags1,
        char flags2,
        const PhongBidirectionalScatteringDistributionFunction *bsdf,
        RayHit *hit,
        bool doRR,
        double *x,
        float *probabilityDensityFunction,
        bool *chose1);

  public:
    CPhotonMapSampler();

    // Sample : newNode gets filled, others may change
    // Return true if the node was filled in, false if path Ends
    // When path ends (absorption) the type of thisNode is adjusted to 'Ends'
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
};

#endif

#endif

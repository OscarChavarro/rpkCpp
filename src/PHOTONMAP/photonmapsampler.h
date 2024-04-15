/**
A sampler specifically designed for use with photon maps.
Specular materials are treated as Fresnel reflectors/refractors.

NO DIFFUSE OR GLOSSY TRANSMITTING SURFACES SUPPORTED YET!
*/

#ifndef __PHOTON_MAP_SAMPLER__
#define __PHOTON_MAP_SAMPLER__

#include "raycasting/raytracing/bsdfsampler.h"
#include "PHOTONMAP/photonmap.h"

class CPhotonMapSampler : public CBsdfSampler {
  protected:
    CPhotonMap *m_photonMap; // To be used for importance sampling

    bool
    fresnelSample(
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x2,
        BSDF_FLAGS flags);

    bool
    gdSample(
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x1,
        double x2,
        bool doRR,
        BSDF_FLAGS flags);

    // Randomly choose between 2 scattering components, using
    // scattered power as probabilities.
    // Returns true a component was chosen, false if absorbed
    static bool
    chooseComponent(
        BSDF_FLAGS flags1,
        BSDF_FLAGS flags2,
        BSDF *bsdf,
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
};

#endif

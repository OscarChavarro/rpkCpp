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

    bool FresnelSample(SimpleRaytracingPathNode *prevNode, SimpleRaytracingPathNode *thisNode,
                       SimpleRaytracingPathNode *newNode, double x_2,
                       BSDFFLAGS flags);

    bool GDSample(SimpleRaytracingPathNode *prevNode, SimpleRaytracingPathNode *thisNode,
                  SimpleRaytracingPathNode *newNode, double x_1, double x_2,
                  bool doRR, BSDFFLAGS flags);

    // Randomly choose between 2 scattering components, using
    // scatteredpower as probabilities.
    // Returns true a component was chosen, false if absorbed
    static bool ChooseComponent(BSDFFLAGS flags1, BSDFFLAGS flags2,
                         BSDF *bsdf, RayHit *hit, bool doRR,
                         double *x, float *pdf, bool *chose1);

public:
    CPhotonMapSampler();

    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path Ends
    //   When path ends (absorption) the type of thisNode is adjusted to 'Ends'

    virtual bool Sample(SimpleRaytracingPathNode *prevNode, SimpleRaytracingPathNode *thisNode,
                        SimpleRaytracingPathNode *newNode, double x_1, double x_2,
                        bool doRR, BSDFFLAGS flags);

    //   virtual double EvalPDF(CPathNode *thisNode, CPathNode *newNode,
    // 			 BSDFFLAGS flags, double *pdf = nullptr,
    // 			 double *pdfRR = nullptr);
    //   virtual double EvalPDFPrev(CPathNode *prevNode,
    // 			     CPathNode *thisNode,
    // 			     CPathNode *newNode,
    // 			     BSDFFLAGS flags,
    // 			     double *pdf, double *pdfRR);

};

#endif // __PHOTON_MAP_SAMPLER__

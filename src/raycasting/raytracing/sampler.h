/**
Generic class for samplers. Samplers operate on
path nodes and have to possible actions :
  - sample : fill in a new path node and evaluate bsdf's and pdf's
             where necessary.
  - connect : connect to sub-paths and evaluate the necessary
              bsdf's and pdf's.
*/

#ifndef __SAMPLER__
#define __SAMPLER__

#include "raycasting/common/pathnode.h"
#include "scene/Background.h"

class Sampler {
  protected:
    virtual bool SampleTransfer(
        Background *sceneBackground,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        Vector3D *dir,
        double pdfDir);

public:
    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path Ends
    //   When path ends (absorption) the type of thisNode is adjusted to 'Ends'
    Sampler() {}
    virtual ~Sampler() {}

    virtual bool
    sample(
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x1,
        double x2,
        bool doRR = false,
        BSDF_FLAGS flags = BSDF_ALL_COMPONENTS) = 0;

    virtual double
    evalPDF(
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        BSDF_FLAGS flags = BSDF_ALL_COMPONENTS,
        double *probabilityDensityFunction = nullptr,
        double *probabilityDensityFunctionRR = nullptr) = 0;
};

/**
Next event samplers provide a few functions to
enumerate different 'next event units' (e.g. light sources
or cameras). This allows to sample all units separately,
f.i. if you want to sample all light sources.

The interface is very simple. I just wanted to be able
to sample all light sources.
*/
class CNextEventSampler : public Sampler {
public:
    // Setting units causes sampling of the activated unit
    // instead of over all units.

    virtual bool ActivateFirstUnit() { return false; }

    // Activate next unit.
    // If no next unit is available:
    //   Returns false and unsets units
    virtual bool ActivateNextUnit() { return false; }
};


/**
A surface sampler is for scattering on surfaces. Here we need
extra parameters to decide if russian roulette is necessary and
flags to indicate what components of the bsdf should be sampled
and evaluated.
*/
class CSurfaceSampler : public Sampler {
  protected:
    bool m_computeFromNextPdf;
    bool m_computeBsdfComponents;

    static void DetermineRayType(SimpleRaytracingPathNode *thisNode, SimpleRaytracingPathNode *newNode, Vector3D *dir);

  public:
    CSurfaceSampler() {
        m_computeFromNextPdf = false;
        m_computeBsdfComponents = false;
    }

    /**
    DoBsdfEval : this just evaluates the bsdf but depending on
    m_computeBsdfComponents uses BsdfEval or BsdfEvalComponents
    Introduced to share code
    */
    inline ColorRgb
    DoBsdfEval(
        BSDF *bsdf,
        RayHit *hit,
        BSDF *inBsdf,
        BSDF *outBsdf,
        Vector3D *in,
        Vector3D *out,
        BSDF_FLAGS flags,
        BsdfComp *bsdfComp) const
    {
        if ( m_computeBsdfComponents ) {
            return (bsdfEvalComponents(bsdf, hit, inBsdf, outBsdf,
                                       in, out, flags, *bsdfComp));
        } else {
            bsdfComp->Clear();
            return (bsdfEval(bsdf, hit, inBsdf, outBsdf,
                             in, out, flags));
        }
    }

    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path Ends
    //   When path ends (absorption) the type of thisNode is adjusted to 'Ends'
    virtual bool
    sample(
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x1,
        double x2,
        bool doRR,
        BSDF_FLAGS flags) = 0;

    // EvalPDF : returns probabilityDensityFunction INCLUDING russian roulette. Separate
    // components can be obtained through probabilityDensityFunction and probabilityDensityFunctionRR params
    virtual double
    evalPDF(
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        BSDF_FLAGS flags,
        double *probabilityDensityFunction = nullptr,
        double *probabilityDensityFunctionRR = nullptr) = 0;

    // Use this for calculating f.i. eyeEndNode->Previous pdf(Next).
    // The newNode is calculated, thisNode should be and end node connecting
    // to another sub path end node. prevNode is that other sub-path
    // endNode.
    virtual double
    EvalPDFPrev(
        SimpleRaytracingPathNode *prevNode,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        BSDF_FLAGS flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR) = 0;

    // bool computeFromNextPdf : if true the surface sampler will
    //   compute pdfFromNext in the prevNode. This is needed for
    //   bidirectional algorithm's
    void SetComputeFromNextPdf(bool computeFromNextPdf) { m_computeFromNextPdf = computeFromNextPdf; }

    void SetComputeBsdfComponents(bool computeBsdfComponents) { m_computeBsdfComponents = computeBsdfComponents; }
};

#endif

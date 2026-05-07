#ifndef __C_SURFACE_SAMPLER__
#define __C_SURFACE_SAMPLER__

#include "raycasting/raytracing/Sampler.h"

/**
A surface sampler is for scattering on surfaces. Here we need
extra parameters to decide if russian roulette is necessary and
flags to indicate what components of the bsdf should be sampled
and evaluated.
*/
class SurfaceSampler : public Sampler {
  protected:
    bool m_computeFromNextPdf;
    bool m_computeBsdfComponents;

    static void DetermineRayType(SimpleRaytracingPathNode *thisNode, SimpleRaytracingPathNode *newNode, const Vector3D *dir);

  public:
    SurfaceSampler() {
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
        const PhongBidirectionalScatteringDistributionFunction *bsdf,
        RayHit *hit,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags,
        BsdfComp *bsdfComp) const
    {
        bool ok = false;
        const ShadingContext context = hit->shadingContext(&ok);

        if ( m_computeBsdfComponents ) {
            if ( bsdf == nullptr ) {
                ColorRgb black;
                black.clear();
                return black;
            } else {
                return bsdf->bsdfEvalComponents(context, inBsdf, outBsdf, in, out, flags, *bsdfComp);
            }
        } else {
            bsdfComp->Clear();
            ColorRgb radiance;
            if ( bsdf == nullptr ) {
                radiance.clear();
            } else {
                radiance = bsdf->evaluate(context, inBsdf, outBsdf, in, out, flags);
            }
            return radiance;
        }
    }

    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path Ends
    //   When path ends (absorption) the type of thisNode is adjusted to 'Ends'
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
        bool doRR,
        char flags) = 0;

    // EvalPDF : returns probabilityDensityFunction INCLUDING russian roulette. Separate
    // components can be obtained through probabilityDensityFunction and probabilityDensityFunctionRR params
    virtual double
    evalPDF(
        Camera *camera,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        char flags,
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
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR) = 0;

    // bool computeFromNextPdf : if true the surface sampler will
    //   compute pdfFromNext in the prevNode. This is needed for
    //   bidirectional algorithm's
    void SetComputeFromNextPdf(bool computeFromNextPdf) { m_computeFromNextPdf = computeFromNextPdf; }

    void SetComputeBsdfComponents(bool computeBsdfComponents) { m_computeBsdfComponents = computeBsdfComponents; }
};

#endif

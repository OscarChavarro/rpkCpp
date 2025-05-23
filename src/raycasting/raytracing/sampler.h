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

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "raycasting/common/pathnode.h"
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
    Sampler() {}
    virtual ~Sampler() {}

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
        char flags = BSDF_ALL_COMPONENTS) = 0;

    virtual double
    evalPDF(
        Camera *camera,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        char flags = BSDF_ALL_COMPONENTS,
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

    static void DetermineRayType(SimpleRaytracingPathNode *thisNode, SimpleRaytracingPathNode *newNode, const Vector3D *dir);

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
        const PhongBidirectionalScatteringDistributionFunction *bsdf,
        RayHit *hit,
        const PhongBidirectionalScatteringDistributionFunction *inBsdf,
        const PhongBidirectionalScatteringDistributionFunction *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        char flags,
        BsdfComp *bsdfComp) const
    {
        if ( m_computeBsdfComponents ) {
            if ( bsdf == nullptr ) {
                ColorRgb black;
                black.clear();
                return black;
            } else {
                return bsdf->bsdfEvalComponents(hit, inBsdf, outBsdf, in, out, flags, *bsdfComp);
            }
        } else {
            bsdfComp->Clear();
            ColorRgb radiance;
            if ( bsdf == nullptr ) {
                radiance.clear();
            } else {
                radiance = bsdf->evaluate(hit, inBsdf, outBsdf, in, out, flags);
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

#endif

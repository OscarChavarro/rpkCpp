/* sampler.H
 *
 * generic class for samplers. Samplers operate on 
 * pathnodes and have to possible actions :
 *   - sample : fill in a new path node and evaluate bsdf's and pdf's
 *              where necessary.
 *   - connect : connect to subpaths and evaluate the necessary
 *               bsdf's and pdf's.
 */

#ifndef _SAMPLER_H_
#define _SAMPLER_H_

#include <cstdio>

#include "raycasting/common/pathnode.h"

// There exists no implementation of this baseclass

class CSampler {
protected:

    // Sample transfer generates a new point on a surface by ray tracing
    // given a certain direction and the pdf for that direction
    // The medium the ray is traveling through must be known and
    // is given by newNode->m_inBsdf.
    // The function fills in newNode: hit, normal, directions, pdf (area)
    //          geometry factor and depth  (rayType is untocuhed)
    // It returns false if no point was found when tracing a ray
    // or if a shading normal anomaly occurs
    virtual bool SampleTransfer(CPathNode *thisNode, CPathNode *newNode,
                                Vector3D *dir, double pdfDir);

public:
    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path Ends
    //   If path ends (absorption) the type of thisNode is adjusted to 'Ends'
    CSampler() {}
    virtual ~CSampler() {}

    virtual bool Sample(CPathNode *prevNode, CPathNode *thisNode,
                        CPathNode *newNode, double x_1, double x_2,
                        bool doRR = false,
                        BSDFFLAGS flags = BSDF_ALL_COMPONENTS) = 0;

    virtual double EvalPDF(CPathNode *thisNode, CPathNode *newNode,
                           BSDFFLAGS flags = BSDF_ALL_COMPONENTS,
                           double *pdf = nullptr, double *pdfRR = nullptr) = 0;
};


// Next event samplers provide a few functions to
// enumerate different 'next event units' (e.g. lightsources
// or cameras). This allows to sample all units separately,
// f.i. if you want to sample all lightsources.

// The interface is very simple. I just wanted to be able
// to sample all lightsources.

class CNextEventSampler : public CSampler {
public:
    // Setting units causes sampling of the activated unit
    // instead of over all units.

    virtual bool ActivateFirstUnit() { return false; }

    // Activate next unit.
    // If no next unit is available:
    //   Returns false and unsets units
    virtual bool ActivateNextUnit() { return false; }

    // Deactivate units, sampling now over all units
    virtual void DeactivateUnits() {}
};


/* A surface sampler is for scattering on surfaces. Here we need
 * extra parameters to decide if russian roulette is necessary and
 * flags to indicate what components of the bsdf should be sampled
 * and evaluated.
 */

class CSurfaceSampler : public CSampler {
protected:
    bool m_computeFromNextPdf;
    bool m_computeBsdfComponents;

    void DetermineRayType(CPathNode *thisNode, CPathNode *newNode, Vector3D *dir);

public:
    CSurfaceSampler() {
        m_computeFromNextPdf = false;
        m_computeBsdfComponents = false;
    }

    // DoBsdfEval : this just evaluates the bsdf but depending on
    // 'm_computeBsdfComponents' uses BsdfEval or BsdfEvalComponents
    // Introduced to share code

    inline COLOR DoBsdfEval(BSDF *bsdf, RayHit *hit, BSDF *inBsdf,
                            BSDF *outBsdf, Vector3D *in, Vector3D *out,
                            BSDFFLAGS flags,
                            CBsdfComp *bsdfComp) {
        if ( m_computeBsdfComponents ) {
            return (BsdfEvalComponents(bsdf, hit, inBsdf, outBsdf,
                                       in, out, flags, *bsdfComp));
        } else {
            bsdfComp->Clear();
            return (BsdfEval(bsdf, hit, inBsdf, outBsdf,
                             in, out, flags));
        }
    }

    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path Ends
    //   If path ends (absorption) the type of thisNode is adjusted to 'Ends'
    virtual bool Sample(CPathNode *prevNode, CPathNode *thisNode,
                        CPathNode *newNode, double x_1, double x_2,
                        bool doRR, BSDFFLAGS flags) = 0;

    // EvalPDF : returns pdf INCLUDING russian roulette. Separate
    // components can be obtained through pdf and pdfRR params
    virtual double EvalPDF(CPathNode *thisNode, CPathNode *newNode,
                           BSDFFLAGS flags, double *pdf = nullptr,
                           double *pdfRR = nullptr) = 0;

    // Use this for calculating f.i. eyeEndNode->Previous pdf(Next).
    // The newNode is calculated, thisNode should be and end node connecting
    // to another sub path end node. prevNode is that other subpath
    // endNode.
    virtual double EvalPDFPrev(CPathNode *prevNode,
                               CPathNode *thisNode,
                               CPathNode *newNode,
                               BSDFFLAGS flags,
                               double *pdf, double *pdfRR) = 0;
    // Configure

    // bool computeFromNextPdf : if true the surface sampler will
    //   compute pdfFromNext in the prevNode. This is needed for
    //   bidirectional algorithm's

    void SetComputeFromNextPdf(bool computeFromNextPdf) { m_computeFromNextPdf = computeFromNextPdf; }

    void SetComputeBsdfComponents(bool computeBsdfComponents) { m_computeBsdfComponents = computeBsdfComponents; }
};

#endif

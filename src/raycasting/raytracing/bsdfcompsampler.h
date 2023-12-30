/* bsdfcompsampler.H
 *
 * pathnode sampler using bsdf component sampling:
 * the result of taking a sample is a direction using
 * one of a number of scattering components and the
 * PDF evaluation only includes these components!
 *
 * This is useful if we need to trace paths where
 * different actions are taken based on the scattering
 * components (e.g. specular vs. diffuse in photon maps)
 *
 * The used scattering component is set in the path node.
 *
 */

#ifndef _BSDFCOMPSAMPLER_H_
#define _BSDFCOMPSAMPLER_H_

#include "material/xxdf.h"
#include "raycasting/raytracing/bsdfsampler.h"

const int MAX_COMP_GROUPS = BSDFCOMPONENTS;

class CBsdfCompSampler : public CBsdfSampler {
protected:
    // A maximum of 6 components groups is allowed. Any group can
    // contain any component (not overlapping!!)
    BSDFFLAGS m_compArray[MAX_COMP_GROUPS];
    int m_compArraySize;
public:
    // Constructor: Up to 6 component groups allowed. If this gets
    // more, we should consider var args...
    CBsdfCompSampler(BSDFFLAGS comp1 = NO_COMPONENTS,
                     BSDFFLAGS comp2 = NO_COMPONENTS,
                     BSDFFLAGS comp3 = NO_COMPONENTS,
                     BSDFFLAGS comp4 = NO_COMPONENTS,
                     BSDFFLAGS comp5 = NO_COMPONENTS,
                     BSDFFLAGS comp6 = NO_COMPONENTS);

    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path Ends
    //   If path ends (absorption) the type of thisNode is adjusted to 'Ends'
    virtual bool Sample(CPathNode *prevNode, CPathNode *thisNode,
                        CPathNode *newNode, double x_1, double x_2,
                        bool doRR, BSDFFLAGS flags);

    //-- PDF eval functions not yet implemented! Do not use
    //-- with Multiple Importance Sampling (== Bidirectional Path Tracing
    //-- && standard stochastic raytracing) only with photonmap method!

    //   virtual double EvalPDF(CPathNode *thisNode, CPathNode *newNode,
    // 			 BSDFFLAGS flags, double *pdf = nullptr,
    // 			 double *pdfRR = nullptr);
    //   virtual double EvalPDFPrev(CPathNode *prevNode,
    // 			     CPathNode *thisNode,
    // 			     CPathNode *newNode,
    // 			     BSDFFLAGS flags,
    // 			     double *pdf, double *pdfRR);

};

#endif // _BSDFCOMPSAMPLER_H_

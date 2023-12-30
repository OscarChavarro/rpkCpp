/* bsdfsampler.H
 *
 * pathnode sampler using bsdf sampling
 */

#ifndef _BSDFSAMPLER_H_
#define _BSDFSAMPLER_H_

#include "raycasting/raytracing/sampler.h"

class CBsdfSampler : public CSurfaceSampler {
public:
    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path Ends
    //   If path ends (absorption) the type of thisNode is adjusted to 'Ends'
    virtual bool Sample(CPathNode *prevNode, CPathNode *thisNode,
                        CPathNode *newNode, double x_1, double x_2,
                        bool doRR, BSDFFLAGS flags);

    // Use this for a N.E.E. : connecting a light node with an eye node
    virtual double EvalPDF(CPathNode *thisNode, CPathNode *newNode,
                           BSDFFLAGS flags, double *pdf = nullptr,
                           double *pdfRR = nullptr);

    // Use this for calculating f.i. eyeEndNode->Previous pdf(Next).
    // The newNode is calculated, thisNode should be and end node connecting
    // to another sub path end node. prevNode is that other subpath
    // endNode.
    virtual double EvalPDFPrev(CPathNode *prevNode,
                               CPathNode *thisNode,
                               CPathNode *newNode,
                               BSDFFLAGS flags,
                               double *pdf, double *pdfRR);

};

#endif // _BSDFSAMPLER_H_

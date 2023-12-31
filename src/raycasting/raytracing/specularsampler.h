/* specularsampler.H
 *
 * pathnode sampler using PERFECT SPECULAR reflection/refraction only
 *
 * ***** BEWARE ***** 
 * This is a 'dirac' sampler, meaning that the pdf
 * for generating the outgoing direction is infinite (dirac)
 * You CANNOT use these samplers together with other samplers
 * for multiple importance sampling !!
 *
 * All pdf evaluations should be multiplied by infinity...
 *
 * I currently use it only in rtstochastic.C for classical raytracing
 *
 */

#ifndef _SPECULARSAMPLER_H_
#define _SPECULARSAMPLER_H_

#include "raycasting/raytracing/sampler.h"

class CSpecularSampler : public CSurfaceSampler {
public:
    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path Ends
    //   If path ends (absorption) the type of thisNode is adjusted to 'Ends'

    // *** 'flags' is used to determine the amount of energy that is
    // reflected/refracted. If there are both reflective AND refractive
    // components, a scattermode is chosen randomly !!

    virtual bool Sample(CPathNode *prevNode, CPathNode *thisNode,
                        CPathNode *newNode, double x_1, double x_2,
                        bool doRR, BSDFFLAGS flags);

    // Use this for a N.E.E. : connecting a light node with an eye node

    // Return value should be multiplied by infinity !!
    virtual double EvalPDF(CPathNode *thisNode, CPathNode *newNode,
                           BSDFFLAGS flags, double *pdf = nullptr,
                           double *pdfRR = nullptr);

    // Use this for calculating f.i. eyeEndNode->Previous pdf(Next).
    // The newNode is calculated, thisNode should be and end node connecting
    // to another sub path end node. prevNode is that other subpath
    // endNode.
    // Return value should be multiplied by infinity !!
    virtual double EvalPDFPrev(CPathNode *prevNode,
                               CPathNode *thisNode,
                               CPathNode *newNode,
                               BSDFFLAGS flags,
                               double *pdf, double *pdfRR);


};

#endif // _SPECULARSAMPLER_H_

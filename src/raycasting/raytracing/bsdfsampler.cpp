#include <cmath>

#include "common/error.h"
#include "material/bsdf.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/bsdfsampler.h"

bool CBsdfSampler::Sample(CPathNode *prevNode, CPathNode *thisNode,
                          CPathNode *newNode, double x_1, double x_2,
                          bool doRR, BSDFFLAGS flags) {
    double pdfDir;

    // Sample direction
    Vector3D dir = BsdfSample(thisNode->m_useBsdf,
                              &thisNode->m_hit,
                              thisNode->m_inBsdf, thisNode->m_outBsdf,
                              &thisNode->m_inDirF,
                              doRR, flags, x_1, x_2,
                              &pdfDir);

    PNAN(pdfDir);

    if ( pdfDir <= EPSILON ) {
        return false;
    } // No good sample

    newNode->m_rracc = thisNode->m_rracc;
    if ( doRR ) {
        COLOR albedo = BsdfScatteredPower(thisNode->m_useBsdf, &thisNode->m_hit,
                                          &thisNode->m_inDirF, flags);
        newNode->m_rracc *= colorAverage(albedo);
    }



    // Reflection Type, changes thisNode->m_rayType and newNode->m_inBsdf
    DetermineRayType(thisNode, newNode, &dir);

    // Transfer
    if ( !SampleTransfer(thisNode, newNode, &dir, pdfDir)) {
        thisNode->m_rayType = Stops;
        return false;
    }

    // Fill in bsdf of current node

    thisNode->m_bsdfEval = DoBsdfEval(thisNode->m_useBsdf,
                                      &thisNode->m_hit,
                                      thisNode->m_inBsdf,
                                      thisNode->m_outBsdf,
                                      &thisNode->m_inDirF,
                                      &newNode->m_inDirT,
                                      flags,
                                      &thisNode->m_bsdfComp);

    // Accumulate scattering components
    thisNode->m_usedComponents = flags;
    newNode->m_accUsedComponents = (thisNode->m_accUsedComponents |
                                    thisNode->m_usedComponents);


    // thisNode->m_bsdfEvalFromNext = thisNode->m_bsdfEvalFromPrev;


    // Fill in probability for previous node

    if ( m_computeFromNextPdf && prevNode ) {
        double cosI = VECTORDOTPRODUCT(thisNode->m_normal,
                                       thisNode->m_inDirF);
        double pdfDirI, pdfRR;

        // prevpdf : new->this->prev pdf evaluation
        // normal direction is handled by the evalpdf routine
        /* -- Are the flags usable in both directions ? -- */
        BsdfEvalPdf(thisNode->m_useBsdf,
                    &thisNode->m_hit,
                    thisNode->m_outBsdf, thisNode->m_inBsdf,
                    &newNode->m_inDirT,
                    &thisNode->m_inDirF,
                    flags, &pdfDirI, &pdfRR);


        PNAN(pdfDirI);
        PNAN(pdfRR);

        prevNode->m_rrPdfFromNext = pdfRR;
        prevNode->m_pdfFromNext = pdfDirI * thisNode->m_G / cosI;
    }

    return true; // Node filled in
}

double CBsdfSampler::EvalPDF(CPathNode *thisNode, CPathNode *newNode,
                             BSDFFLAGS flags, double *pdf, double *pdfRR) {
    double pdfDir, dist2, dist, cosa, pdfH, pdfRRH;
    Vector3D outDir;

    if ( pdf == nullptr ) {
        pdf = &pdfH;
    }
    if ( pdfRR == nullptr ) {
        pdfRR = &pdfRRH;
    }

    /* -- more efficient with extra params ?? -- */
    VECTORSUBTRACT(newNode->m_hit.point, thisNode->m_hit.point, outDir);
    dist2 = VECTORNORM2(outDir);
    dist = sqrt(dist2);
    VECTORSCALEINVERSE(dist, outDir, outDir);

    // Beware : NOT RECIPROKE !!!!!!
    BsdfEvalPdf(thisNode->m_useBsdf,
                &thisNode->m_hit,
                thisNode->m_inBsdf, thisNode->m_outBsdf,
                &thisNode->m_inDirF, &outDir,
                flags, &pdfDir, pdfRR);

    // To area measure
    cosa = -VECTORDOTPRODUCT(outDir, newNode->m_normal);

    *pdf = pdfDir * cosa / dist2;

    return *pdf * *pdfRR;
}


double CBsdfSampler::EvalPDFPrev(CPathNode *prevNode,
                                 CPathNode *thisNode, CPathNode */*newNode*/,
                                 BSDFFLAGS flags,
                                 double *pdf, double *pdfRR) {
    double pdfDir, cosb, pdfH, pdfRRH;
    Vector3D outDir;

    if ( pdf == nullptr ) {
        pdf = &pdfH;
    }
    if ( pdfRR == nullptr ) {
        pdfRR = &pdfRRH;
    }

    /* -- more efficient with extra params ?? -- */

    VECTORSUBTRACT(prevNode->m_hit.point, thisNode->m_hit.point, outDir);
    VECTORNORMALIZE(outDir);

    // Beware : NOT RECIPROKE !!!!!!
    BsdfEvalPdf(thisNode->m_useBsdf,
                &thisNode->m_hit,
                thisNode->m_outBsdf, thisNode->m_inBsdf,
                &outDir, &thisNode->m_inDirF,
                flags, &pdfDir, pdfRR);

    // To area measure

    cosb = VECTORDOTPRODUCT(thisNode->m_inDirF, thisNode->m_normal);

    *pdf = pdfDir * thisNode->m_G / cosb;

    return *pdf * *pdfRR;
}




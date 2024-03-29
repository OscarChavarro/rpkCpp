#include <cmath>

#include "common/error.h"
#include "material/bsdf.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/bsdfsampler.h"

bool CBsdfSampler::sample(SimpleRaytracingPathNode *prevNode, SimpleRaytracingPathNode *thisNode,
                          SimpleRaytracingPathNode *newNode, double x_1, double x_2,
                          bool doRR, BSDF_FLAGS flags) {
    double pdfDir;

    // Sample direction
    Vector3D dir = bsdfSample(thisNode->m_useBsdf,
                              &thisNode->m_hit,
                              thisNode->m_inBsdf, thisNode->m_outBsdf,
                              &thisNode->m_inDirF,
                              doRR, flags, x_1, x_2,
                              &pdfDir);

    PNAN(pdfDir);

    if ( pdfDir <= EPSILON ) {
        return false;
    } // No good sample

    newNode->accumulatedRussianRouletteFactors = thisNode->accumulatedRussianRouletteFactors;
    if ( doRR ) {
        COLOR albedo = bsdfScatteredPower(thisNode->m_useBsdf, &thisNode->m_hit,
                                          &thisNode->m_inDirF, flags);
        newNode->accumulatedRussianRouletteFactors *= colorAverage(albedo);
    }



    // Reflection Type, changes thisNode->m_rayType and newNode->m_inBsdf
    DetermineRayType(thisNode, newNode, &dir);

    // Transfer
    if ( !SampleTransfer(thisNode, newNode, &dir, pdfDir)) {
        thisNode->m_rayType = STOPS;
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
    newNode->m_accUsedComponents = static_cast<BSDF_FLAGS>(thisNode->m_accUsedComponents |
                                                           thisNode->m_usedComponents);


    // thisNode->m_bsdfEvalFromNext = thisNode->m_bsdfEvalFromPrev;


    // Fill in probability for previous node

    if ( m_computeFromNextPdf && prevNode ) {
        double cosI = vectorDotProduct(thisNode->m_normal,
                                       thisNode->m_inDirF);
        double pdfDirI, pdfRR;

        // prevpdf : new->this->prev pdf evaluation
        // normal direction is handled by the evalpdf routine
        /* -- Are the flags usable in both directions ? -- */
        bsdfEvalPdf(thisNode->m_useBsdf,
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

double
CBsdfSampler::evalPDF(
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    BSDF_FLAGS flags,
    double *pdf,
    double *pdfRR)
{
    double pdfDir;
    double dist2;
    double dist;
    double cosa;
    double pdfH;
    double pdfRRH;
    Vector3D outDir;

    if ( pdf == nullptr ) {
        pdf = &pdfH;
    }
    if ( pdfRR == nullptr ) {
        pdfRR = &pdfRRH;
    }

    /* -- more efficient with extra params ?? -- */
    vectorSubtract(newNode->m_hit.point, thisNode->m_hit.point, outDir);
    dist2 = vectorNorm2(outDir);
    dist = std::sqrt(dist2);
    vectorScaleInverse((float)dist, outDir, outDir);

    // Beware : NOT RECIPROKE !!!!!!
    bsdfEvalPdf(thisNode->m_useBsdf,
                &thisNode->m_hit,
                thisNode->m_inBsdf, thisNode->m_outBsdf,
                &thisNode->m_inDirF, &outDir,
                flags, &pdfDir, pdfRR);

    // To area measure
    cosa = -vectorDotProduct(outDir, newNode->m_normal);

    *pdf = pdfDir * cosa / dist2;

    return *pdf * *pdfRR;
}


double CBsdfSampler::EvalPDFPrev(SimpleRaytracingPathNode *prevNode,
                                 SimpleRaytracingPathNode *thisNode, SimpleRaytracingPathNode */*newNode*/,
                                 BSDF_FLAGS flags,
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

    vectorSubtract(prevNode->m_hit.point, thisNode->m_hit.point, outDir);
    vectorNormalize(outDir);

    // Beware : NOT RECIPROKE !!!!!!
    bsdfEvalPdf(thisNode->m_useBsdf,
                &thisNode->m_hit,
                thisNode->m_outBsdf, thisNode->m_inBsdf,
                &outDir, &thisNode->m_inDirF,
                flags, &pdfDir, pdfRR);

    // To area measure

    cosb = vectorDotProduct(thisNode->m_inDirF, thisNode->m_normal);

    *pdf = pdfDir * thisNode->m_G / cosb;

    return *pdf * *pdfRR;
}




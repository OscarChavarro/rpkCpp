#include "common/error.h"
#include "material/bsdf.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/specularsampler.h"

bool CSpecularSampler::Sample(CPathNode *prevNode, CPathNode *thisNode,
                              CPathNode *newNode, double x_1, double x_2,
                              bool doRR, BSDFFLAGS flags) {
    Vector3D dir;
    double pdfDir = 1.0;
    bool reflect;

    // Choose a scattering mode : reflection vs. refraction

    COLOR reflectance = bsdfReflectance(thisNode->m_useBsdf,
                                        &thisNode->m_hit,
                                        &thisNode->m_hit.normal,
                                        GETBRDFFLAGS(flags));
    COLOR transmittance = bsdfScatteredPower(thisNode->m_useBsdf,
                                            &thisNode->m_hit,
                                            &thisNode->m_hit.normal,
                                            GETBTDFFLAGS(flags));

    float avgReflectance = colorAverage(reflectance);
    float avgTransmittance = colorAverage(transmittance);

    float avgScattering = avgReflectance + avgTransmittance;

    if ( avgScattering < EPSILON ) {
        return false;
    }

    // Just using one of the random numbers for scatter mode choice
    if ( x_1 * avgScattering < avgReflectance ) {
        // REFLECT

        reflect = true;
        pdfDir *= avgReflectance / avgScattering;

        dir = IdealReflectedDirection(&thisNode->m_inDirT,
                                      &thisNode->m_normal);
    } else {
        // REFRACT

        int dummyInt;
        REFRACTIONINDEX inIndex, outIndex;

        reflect = false;
        pdfDir *= avgTransmittance / avgScattering;

        bsdfIndexOfRefraction(thisNode->m_inBsdf, &inIndex);
        bsdfIndexOfRefraction(thisNode->m_outBsdf, &outIndex);

        dir = IdealRefractedDirection(&thisNode->m_inDirT,
                                      &thisNode->m_normal,
                                      inIndex, outIndex, &dummyInt);
    }

    PNAN(pdfDir);
    if ( pdfDir < EPSILON ) {
        return false;
    }

    // Reflection Type, changes thisNode->m_rayType and newNode->m_inBsdf
    DetermineRayType(thisNode, newNode, &dir);

    // Transfer
    if ( !SampleTransfer(thisNode, newNode, &dir, pdfDir)) {
        thisNode->m_rayType = Stops;
        return false;
    }


    // Fill in bsdf evaluation. This is just reflectance or transmittance
    // No components yet here !!

    if ( reflect ) {
        thisNode->m_bsdfEval = reflectance;
    } else {
        thisNode->m_bsdfEval = transmittance;
    }

    // Fill in probability for previous node, normally not yet used

    if ( m_computeFromNextPdf && prevNode ) {
        double cosI = VECTORDOTPRODUCT(thisNode->m_normal,
                                       thisNode->m_inDirF);
        double pdfDirI, pdfRR;

        // prevpdf : new->this->prev pdf evaluation
        // normal direction is handled by the evalpdf routine

        pdfRR = avgScattering;
        pdfDirI = pdfDir;

        prevNode->m_rrPdfFromNext = pdfRR;
        prevNode->m_pdfFromNext = pdfDirI * thisNode->m_G / cosI;
    }

    return true; // Node filled in
}


double CSpecularSampler::EvalPDF(CPathNode *thisNode, CPathNode *newNode,
                                 BSDFFLAGS flags, double *pdf, double *pdfRR) {
    if ( pdf ) {
        *pdf = 0;
    }
    if ( pdfRR ) {
        *pdfRR = 0;
    }

    return 0.;  // Specular reflection can only be done by sampling!
}


double CSpecularSampler::EvalPDFPrev(CPathNode *prevNode,
                                     CPathNode *thisNode, CPathNode */*newNode*/,
                                     BSDFFLAGS flags,
                                     double *pdf, double *pdfRR) {
    *pdf = 0.;
    *pdfRR = 0.;

    return 0.;
}



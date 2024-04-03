#include "common/error.h"
#include "material/bsdf.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/specularsampler.h"

bool
CSpecularSampler::sample(
    SimpleRaytracingPathNode *prevNode,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    bool doRR,
    BSDF_FLAGS flags)
{
    Vector3D dir;
    double pdfDir = 1.0;
    bool reflect;

    // Choose a scattering mode : reflection vs. refraction
    ColorRgb reflectance = bsdfReflectance(thisNode->m_useBsdf,
                                           &thisNode->m_hit,
                                           &thisNode->m_hit.normal,
                                           GET_BRDF_FLAGS(flags));
    ColorRgb transmittance = bsdfScatteredPower(thisNode->m_useBsdf,
                                                &thisNode->m_hit,
                                                &thisNode->m_hit.normal,
                                                GET_BTDF_FLAGS(flags));

    float avgReflectance = colorAverage(reflectance);
    float avgTransmittance = colorAverage(transmittance);

    float avgScattering = avgReflectance + avgTransmittance;

    if ( avgScattering < EPSILON ) {
        return false;
    }

    // Just using one of the random numbers for scatter mode choice
    if ( x1 * avgScattering < avgReflectance ) {
        // REFLECT
        reflect = true;
        pdfDir *= avgReflectance / avgScattering;

        dir = idealReflectedDirection(&thisNode->m_inDirT, &thisNode->m_normal);
    } else {
        // REFRACT
        int dummyInt;
        RefractionIndex inIndex{};
        RefractionIndex outIndex{};

        reflect = false;
        pdfDir *= avgTransmittance / avgScattering;

        bsdfIndexOfRefraction(thisNode->m_inBsdf, &inIndex);
        bsdfIndexOfRefraction(thisNode->m_outBsdf, &outIndex);

        dir = idealRefractedDirection(&thisNode->m_inDirT,
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
    if ( !SampleTransfer(thisNode, newNode, &dir, pdfDir) ) {
        thisNode->m_rayType = STOPS;
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
        double cosI = vectorDotProduct(thisNode->m_normal,
                                       thisNode->m_inDirF);
        double pdfDirI, pdfRR;

        // prevPdf : new->this->prev pdf evaluation
        // normal direction is handled by the evalPdf routine

        pdfRR = avgScattering;
        pdfDirI = pdfDir;

        prevNode->m_rrPdfFromNext = pdfRR;
        prevNode->m_pdfFromNext = pdfDirI * thisNode->m_G / cosI;
    }

    return true; // Node filled in
}

double
CSpecularSampler::evalPDF(
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    BSDF_FLAGS flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR)
{
    if ( probabilityDensityFunction ) {
        *probabilityDensityFunction = 0;
    }
    if ( probabilityDensityFunctionRR ) {
        *probabilityDensityFunctionRR = 0;
    }

    return 0.0; // Specular reflection can only be done by sampling!
}


double
CSpecularSampler::EvalPDFPrev(
    SimpleRaytracingPathNode *prevNode,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode */*newNode*/,
    BSDF_FLAGS flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR)
{
    *probabilityDensityFunction = 0.0;
    *probabilityDensityFunctionRR = 0.0;

    return 0.0;
}

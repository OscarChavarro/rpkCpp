#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "common/error.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/specularsampler.h"

bool
CSpecularSampler::sample(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *prevNode,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    bool doRR,
    char flags)
{
    Vector3D dir;
    double pdfDir = 1.0;
    bool reflect;

    // Choose a scattering mode : reflection vs. refraction
    ColorRgb reflectance;
    reflectance.clear();
    if ( thisNode->m_useBsdf != nullptr ) {
        reflectance = thisNode->m_useBsdf->splitBsdfScatteredPower(
            &thisNode->m_hit,
            GET_BRDF_FLAGS(flags));
    }
    ColorRgb transmittance;
    transmittance.clear();
    if ( thisNode->m_useBsdf != nullptr ) {
        transmittance = thisNode->m_useBsdf->splitBsdfScatteredPower(
            &thisNode->m_hit,
            GET_BTDF_FLAGS(flags));
    }

    float avgReflectance = reflectance.average();
    float avgTransmittance = transmittance.average();
    float avgScattering = avgReflectance + avgTransmittance;

    if ( avgScattering < Numeric::EPSILON ) {
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
        bool dummyBoolean;
        RefractionIndex inIndex{};
        RefractionIndex outIndex{};

        reflect = false;
        pdfDir *= avgTransmittance / avgScattering;

        if ( thisNode->m_inBsdf == nullptr ) {
            inIndex.set(1.0, 0.0); // Vacuum
        } else {
            thisNode->m_inBsdf->indexOfRefraction(&inIndex);
        }

        if ( thisNode->m_outBsdf == nullptr ) {
            outIndex.set(1.0, 0.0); // Vacuum
        } else {
            thisNode->m_outBsdf->indexOfRefraction(&outIndex);
        }

        dir = idealRefractedDirection(
            &thisNode->m_inDirT,
            &thisNode->m_normal,
            inIndex,
            outIndex,
            &dummyBoolean);
    }

    if ( pdfDir < Numeric::EPSILON ) {
        return false;
    }

    // Reflection Type, changes thisNode->m_rayType and newNode->m_inBsdf
    DetermineRayType(thisNode, newNode, &dir);

    // Transfer
    if ( !sampleTransfer(
            sceneVoxelGrid,
            sceneBackground,
            thisNode,
            newNode,
            &dir,
            pdfDir) ) {
        thisNode->m_rayType = PathRayType::STOPS;
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
        double cosI = thisNode->m_normal.dotProduct(thisNode->m_inDirF);
        double pdfDirI;
        double pdfRR;

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
    Camera *camera,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    char flags,
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
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR)
{
    *probabilityDensityFunction = 0.0;
    *probabilityDensityFunctionRR = 0.0;

    return 0.0;
}

#endif

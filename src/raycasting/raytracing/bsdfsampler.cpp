#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include <cmath>

#include "common/error.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/bsdfsampler.h"

bool
CBsdfSampler::sample(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *prevNode,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    bool doRR,
    BSDF_FLAGS flags)
{
    double pdfDir;

    // Sample direction
    Vector3D dir = {0.0, 0.0, 0.0};
    pdfDir = 0.0;

    if ( thisNode->m_useBsdf != nullptr ) {
        dir = thisNode->m_useBsdf->sample(
            &thisNode->m_hit,
            thisNode->m_inBsdf,
            thisNode->m_outBsdf,
            &thisNode->m_inDirF,
            doRR,
            flags,
            x1,
            x2,
            &pdfDir);
    }

    if ( pdfDir <= EPSILON ) {
        // No good sample
        return false;
    }

    newNode->accumulatedRussianRouletteFactors = thisNode->accumulatedRussianRouletteFactors;
    if ( doRR ) {
        ColorRgb albedo;
        albedo.clear();
        if ( thisNode->m_useBsdf != nullptr ) {
            albedo = thisNode->m_useBsdf->splitBsdfScatteredPower(&thisNode->m_hit, flags);
        }
        newNode->accumulatedRussianRouletteFactors *= albedo.average();
    }

    // Reflection Type, changes thisNode->m_rayType and newNode->m_inBsdf
    DetermineRayType(thisNode, newNode, &dir);

    // Transfer
    if ( !sampleTransfer(sceneVoxelGrid, sceneBackground, thisNode, newNode, &dir, pdfDir) ) {
        thisNode->m_rayType = STOPS;
        return false;
    }

    // Fill in bsdf of current node
    thisNode->m_bsdfEval = DoBsdfEval(
        thisNode->m_useBsdf,
        &thisNode->m_hit,
        thisNode->m_inBsdf,
        thisNode->m_outBsdf,
        &thisNode->m_inDirF,
        &newNode->m_inDirT,
        flags,
        &thisNode->m_bsdfComp);

    // Accumulate scattering components
    thisNode->m_usedComponents = flags;
    newNode->m_accUsedComponents = static_cast<BSDF_FLAGS>(thisNode->m_accUsedComponents | thisNode->m_usedComponents);

    // Fill in probability for previous node
    if ( m_computeFromNextPdf && prevNode ) {
        double cosI = thisNode->m_normal.dotProduct(thisNode->m_inDirF);
        double pdfDirI = 0.0;
        double pdfRR = 0.0;

        if ( thisNode->m_useBsdf != nullptr ) {
            // prevpdf : new->this->prev pdf evaluation
            // normal direction is handled by the evalpdf routine
            // Are the flags usable in both directions?
            thisNode->m_useBsdf->evaluateProbabilityDensityFunction(
                &thisNode->m_hit,
                thisNode->m_outBsdf,
                thisNode->m_inBsdf,
                &newNode->m_inDirT,
                &thisNode->m_inDirF,
                flags,
                &pdfDirI,
                &pdfRR);
        }

        prevNode->m_rrPdfFromNext = pdfRR;
        prevNode->m_pdfFromNext = pdfDirI * thisNode->m_G / cosI;
    }

    return true; // Node filled in
}

double
CBsdfSampler::evalPDF(
    Camera *camera,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    BSDF_FLAGS flags,
    double *pdf,
    double *pdfRR)
{
    double pdfH;
    double pdfRRH;

    if ( pdf == nullptr ) {
        pdf = &pdfH;
    }
    if ( pdfRR == nullptr ) {
        pdfRR = &pdfRRH;
    }

    // More efficient with extra params?
    double dist2;
    double dist;
    Vector3D outDir;

    outDir.subtraction(newNode->m_hit.getPoint(), thisNode->m_hit.getPoint());
    dist2 = outDir.norm2();
    dist = java::Math::sqrt(dist2);
    outDir.inverseScaledCopy((float)dist, outDir, EPSILON_FLOAT);

    // Beware : NOT RECIPROKE!
    double pdfDir;
    pdfDir = 0;
    *pdfRR = 0;
    if ( thisNode->m_useBsdf != nullptr ) {
        thisNode->m_useBsdf->evaluateProbabilityDensityFunction(
            &thisNode->m_hit,
            thisNode->m_inBsdf,
            thisNode->m_outBsdf,
            &thisNode->m_inDirF,
            &outDir,
            flags,
            &pdfDir,
            pdfRR);
    }

    // To area measure
    double cosA;

    cosA = -outDir.dotProduct(newNode->m_normal);

    *pdf = pdfDir * cosA / dist2;

    return *pdf * *pdfRR;
}

double
CBsdfSampler::EvalPDFPrev(
    SimpleRaytracingPathNode *prevNode,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode */*newNode*/,
    BSDF_FLAGS flags,
    double *pdf,
    double *pdfRR)
{
    double pdfDir;
    double cosB;
    double pdfH;
    double pdfRRH;
    Vector3D outDir;

    if ( pdf == nullptr ) {
        pdf = &pdfH;
    }
    if ( pdfRR == nullptr ) {
        pdfRR = &pdfRRH;
    }

    // More efficient with extra params?
    outDir.subtraction(prevNode->m_hit.getPoint(), thisNode->m_hit.getPoint());
    outDir.normalize(EPSILON_FLOAT);

    // Beware : NOT RECIPROCAL!
    pdfDir = 0.0;
    *pdfRR = 0.0;
    if ( thisNode->m_useBsdf != nullptr ) {
        thisNode->m_useBsdf->evaluateProbabilityDensityFunction(
            &thisNode->m_hit,
            thisNode->m_outBsdf,
            thisNode->m_inBsdf,
            &outDir,
            &thisNode->m_inDirF,
            flags,
            &pdfDir,
            pdfRR);
    }

    // To area measure
    cosB = thisNode->m_inDirF.dotProduct(thisNode->m_normal);

    *pdf = pdfDir * thisNode->m_G / cosB;

    return *pdf * *pdfRR;
}

#endif




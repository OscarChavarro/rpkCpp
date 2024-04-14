#include <cmath>

#include "common/error.h"
#include "material/spherical.h"
#include "material/Material.h"
#include "material/PhongEmittanceDistributionFunction.h"
#include "raycasting/bidirectionalRaytracing/LightDirSampler.h"
#include "scene/scene.h"

/**
sample : newNode gets filled, others may change
*/
bool
LightDirSampler::sample(
    SimpleRaytracingPathNode *prevNode/*prevNode*/,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    bool /* doRR */doRR,
    BSDF_FLAGS /* flags */flags)
{
    double pdfDir;

    if ( !thisNode->m_hit.material->edf ) {
        logError("CLightDirSampler::sample", "No EDF");
        return false;
    }

    // Sample a light direction
    Vector3D dir = edfSample(thisNode->m_hit.material->edf,
                             &thisNode->m_hit, DIFFUSE_COMPONENT,
                             x1, x2, &thisNode->m_bsdfEval,
                             &pdfDir);

    if ( pdfDir < EPSILON ) {
        return false;
    } // Zero probabilityDensityFunction event, no valid sample

    // Determine ray type
    thisNode->m_rayType = STARTS;
    newNode->m_inBsdf = thisNode->m_outBsdf; // Light can be placed in a medium

    // Transfer
    if ( !sampleTransfer(GLOBAL_scene_background, thisNode, newNode, &dir, pdfDir) ) {
        thisNode->m_rayType = STOPS;
        return false;
    }

    // thisNode->m_bsdfEval = EdfEval(thisNode->m_hit.material->edf,
    // &thisNode->m_hit,
    // &(newNode->m_inDirT),
    // DIFFUSE_COMPONENT, (double *)0);

    /* -- Diffuse lights only, put this correctly in 'DoBsdfEval' -- */
    thisNode->m_bsdfComp.Clear();
    thisNode->m_bsdfComp.Fill(thisNode->m_bsdfEval, BRDF_DIFFUSE_COMPONENT);

    // Component propagation
    thisNode->m_usedComponents = NO_COMPONENTS; // the light...
    newNode->m_accUsedComponents = static_cast<BSDF_FLAGS>(thisNode->m_accUsedComponents |
                                                           thisNode->m_usedComponents);

    newNode->accumulatedRussianRouletteFactors = thisNode->accumulatedRussianRouletteFactors;

    return true;
}

double
LightDirSampler::evalPDF(
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    BSDF_FLAGS /*flags*/,
    double * /*probabilityDensityFunction*/,
    double * /*probabilityDensityFunctionRR*/)
{
    double pdfDir;
    double cosA;
    double dist;
    double dist2;
    Vector3D outDir;

    if ( !thisNode->m_hit.material->edf ) {
        logError("CLightDirSampler::evalPdf", "No EDF");
        return false;
    }
    /* -- more efficient with extra params ?? -- */

    vectorSubtract(newNode->m_hit.point, thisNode->m_hit.point, outDir);
    dist2 = vectorNorm2(outDir);
    dist = std::sqrt(dist2);
    vectorScaleInverse((float)dist, outDir, outDir);

    // EDF sampling
    edfEval(thisNode->m_hit.material->edf,
            &thisNode->m_hit, &outDir, DIFFUSE_COMPONENT, &pdfDir);

    if ( pdfDir < 0.0 ) {
        return 0.0;
    }  // Back face of a light does not radiate !

    cosA = -vectorDotProduct(outDir, newNode->m_normal);

    return pdfDir * cosA / dist2;
}

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LightDirSampler.h"

/**
sample : newNode gets filled, others may change
*/
bool
LightDirSampler::sample(
    Camera */*camera*/,
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneSceneQueryWorkspace,
    Background *sceneBackground,
    SimpleRaytracingPathNode */*prevNode*/,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    bool /*doRR*/,
    char /*flags*/)
{
    double pdfDir = 0.0;

    if ( !thisNode->m_hit.getMaterial()->getEdf() ) {
        Logger::reportMessage("LightDirSampler", Logger::ERROR, "CLightDirSampler::sample", "No EDF");
        return false;
    }

    // Sample a light direction
    Vector3Dd dir(0.0, 0.0, 0.0);
    if ( thisNode->m_hit.getMaterial()->getEdf() != NULL ) {
        ShadingContext context = thisNode->m_hit.shadingContext(NULL);
        dir = thisNode->m_hit.getMaterial()->getEdf()->phongEdfSample(&context, DIFFUSE_COMPONENT, x1, x2, &thisNode->m_bsdfEval, &pdfDir);
    }

    if ( pdfDir < VSDK::EPSILON ) {
        return false;
    } // Zero probabilityDensityFunction event, no valid sample

    // Determine ray type
    thisNode->m_rayType = STARTS;
    newNode->m_inBsdf = thisNode->m_outBsdf; // Light can be placed in a medium

    // Transfer
    if ( !sampleTransfer(sceneQuery, sceneSceneQueryWorkspace, sceneBackground, thisNode, newNode, &dir, pdfDir) ) {
        thisNode->m_rayType = STOPS;
        return false;
    }

    // Diffuse lights only, put this correctly in 'bsdfEval'
    thisNode->m_bsdfComp.Clear();
    thisNode->m_bsdfComp.Fill(thisNode->m_bsdfEval, BRDF_DIFFUSE_COMPONENT);

    // Component propagation
    thisNode->m_usedComponents = XxdfComponentFlagInfo::NO_COMPONENTS; // the light...
    newNode->m_accUsedComponents = ((char)(
        thisNode->m_accUsedComponents | thisNode->m_usedComponents));

    newNode->accmlRssnRlttFctrs = thisNode->accmlRssnRlttFctrs;

    return true;
}

double
LightDirSampler::evalPDF(
    Camera */*camera*/,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    char /*flags*/,
    double * /*probabilityDensityFunction*/,
    double * /*probabilityDensityFunctionRR*/)
{
    double pdfDir;
    double cosA;
    double dist;
    double dist2;
    Vector3Dd outDir;

    if ( !thisNode->m_hit.getMaterial()->getEdf() ) {
        Logger::reportMessage("LightDirSampler", Logger::ERROR, "CLightDirSampler::evalPdf", "No EDF");
        return false;
    }
    // More efficient with extra params?

    outDir = (newNode->m_hit.getPoint()).subtract(thisNode->m_hit.getPoint());
    dist2 = outDir.lengthSquared();
    dist = Math::sqrt(dist2);
    outDir = outDir.multiplyByInverse(((double)(dist)), Numeric::EPSILON);

    // EDF sampling
    if ( thisNode->m_hit.getMaterial()->getEdf() == NULL ) {
        pdfDir = 0.0;
    } else {
        ShadingContext context = thisNode->m_hit.shadingContext(NULL);
        thisNode->m_hit.getMaterial()->getEdf()->phongEdfEval(&context, &outDir, DIFFUSE_COMPONENT, &pdfDir);
    }

    if ( pdfDir < 0.0 ) {
        // Back face of a light does not radiate !
        return 0.0;
    }

    cosA = -outDir.dotProduct(newNode->m_normal);

    return pdfDir * cosA / dist2;
}

#endif

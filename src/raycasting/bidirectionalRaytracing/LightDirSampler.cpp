#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "java/lang/Math.h"
#include "common/error.h"
#include "common/linealAlgebra/CoordinateSystem.h"
#include "raycasting/bidirectionalRaytracing/LightDirSampler.h"

/**
sample : newNode gets filled, others may change
*/
bool
LightDirSampler::sample(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *prevNode,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    bool /* doRR */doRR,
    char /* flags */flags)
{
    double pdfDir = 0.0;

    if ( !thisNode->m_hit.getMaterial()->getEdf() ) {
        logError("CLightDirSampler::sample", "No EDF");
        return false;
    }

    // Sample a light direction
    Vector3D dir(0.0f, 0.0f, 0.0f);
    if ( thisNode->m_hit.getMaterial()->getEdf() != nullptr ) {
        dir = thisNode->m_hit.getMaterial()->getEdf()->phongEdfSample(&thisNode->m_hit, DIFFUSE_COMPONENT, x1, x2, &thisNode->m_bsdfEval, &pdfDir);
    }

    if ( pdfDir < Numeric::EPSILON ) {
        return false;
    } // Zero probabilityDensityFunction event, no valid sample

    // Determine ray type
    thisNode->m_rayType = PathRayType::STARTS;
    newNode->m_inBsdf = thisNode->m_outBsdf; // Light can be placed in a medium

    // Transfer
    if ( !sampleTransfer(sceneVoxelGrid, sceneBackground, thisNode, newNode, &dir, pdfDir) ) {
        thisNode->m_rayType = PathRayType::STOPS;
        return false;
    }

    // Diffuse lights only, put this correctly in 'bsdfEval'
    thisNode->m_bsdfComp.Clear();
    thisNode->m_bsdfComp.Fill(thisNode->m_bsdfEval, BRDF_DIFFUSE_COMPONENT);

    // Component propagation
    thisNode->m_usedComponents = NO_COMPONENTS; // the light...
    newNode->m_accUsedComponents = static_cast<char>(
        thisNode->m_accUsedComponents | thisNode->m_usedComponents);

    newNode->accumulatedRussianRouletteFactors = thisNode->accumulatedRussianRouletteFactors;

    return true;
}

double
LightDirSampler::evalPDF(
    Camera *camera,
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
    Vector3D outDir;

    if ( !thisNode->m_hit.getMaterial()->getEdf() ) {
        logError("CLightDirSampler::evalPdf", "No EDF");
        return false;
    }
    // More efficient with extra params?

    outDir.subtraction(newNode->m_hit.getPoint(), thisNode->m_hit.getPoint());
    dist2 = outDir.norm2();
    dist = java::Math::sqrt(dist2);
    outDir.inverseScaledCopy((float) dist, outDir, Numeric::EPSILON_FLOAT);

    // EDF sampling
    if ( thisNode->m_hit.getMaterial()->getEdf() == nullptr ) {
        pdfDir = 0.0;
    } else {
        thisNode->m_hit.getMaterial()->getEdf()->phongEdfEval(&thisNode->m_hit, &outDir, DIFFUSE_COMPONENT, &pdfDir);
    }

    if ( pdfDir < 0.0 ) {
        // Back face of a light does not radiate !
        return 0.0;
    }

    cosA = -outDir.dotProduct(newNode->m_normal);

    return pdfDir * cosA / dist2;
}

#endif

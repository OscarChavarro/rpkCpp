#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "common/error.h"
#include "scene/Camera.h"
#include "raycasting/raytracing/eyesampler.h"

bool
CEyeSampler::sample(
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
    if ( prevNode != nullptr || thisNode != nullptr ) {
        logWarning("CEyeSampler::sample", "Not first node in path ?!");
    }

    // Just fill in newNode with camera data. Appropriate pdf fields are set to 1

    newNode->m_depth = 0; // We expect this to be the first node in an eye path
    newNode->m_rayType = PathRayType::STOPS;

    // Choose eye : N/A
    // Choose point on eye : N/A

    // Fake a hit record
    RayHit *hit = &newNode->m_hit;

    hit->init(nullptr, &camera->eyePosition, &camera->Z, nullptr);
    hit->setNormal(&camera->Z);
    unsigned int newFlags = hit->getFlags() | RayHitFlag::NORMAL | RayHitFlag::SHADING_FRAME;
    hit->setFlags(newFlags);
    hit->setShadingFrame(&camera->X, &camera->Y, &camera->Z);

    newNode->m_normal.copy(newNode->m_hit.getNormal());
    newNode->m_G = 1.0;

    // outDir's not filled in
    newNode->m_pdfFromPrev = 1.0; // Differential eye area cancels out with computing the flux

    newNode->m_pdfFromNext = 0.0; // Eye cannot be hit accidentally, this pdf is never used

    newNode->m_useBsdf = nullptr;
    newNode->m_inBsdf = nullptr;
    newNode->m_outBsdf = nullptr;

    // Component propagation
    newNode->m_accUsedComponents = NO_COMPONENTS; // Eye had no accumulated comps.

    newNode->accumulatedRussianRouletteFactors = 1.0; // No russian roulette

    return true;
}

double
CEyeSampler::evalPDF(
    Camera * /*camera*/,
    SimpleRaytracingPathNode */*thisNode*/,
    SimpleRaytracingPathNode */*newNode*/,
    char /*flags*/,
    double * /*probabilityDensityFunction*/,
    double * /*probabilityDensityFunctionRR*/) {
    return 1.0;
}

#endif

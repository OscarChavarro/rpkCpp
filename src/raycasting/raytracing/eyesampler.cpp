#include "common/error.h"
#include "scene/Camera.h"
#include "scene/scene.h"
#include "raycasting/raytracing/eyesampler.h"

bool CEyeSampler::sample(SimpleRaytracingPathNode *prevNode, SimpleRaytracingPathNode *thisNode,
                         SimpleRaytracingPathNode *newNode, double /*x_1*/x1, double /*x_2*/x2,
                         bool /* doRR */doRR, BSDF_FLAGS /* flags */flags) {
    if ( prevNode != nullptr || thisNode != nullptr ) {
        logWarning("CEyeSampler::sample", "Not first node in path ?!");
    }

    // Just fill in newNode with camera data. Appropiate pdf fields are
    // are set to 1

    newNode->m_depth = 0;  // We expect this to be the first node in an eye path
    newNode->m_rayType = STOPS;

    // Choose eye : N/A
    // Choose point on eye : N/A

    // Fake a hit record

    RayHit *hit = &newNode->m_hit;

    hitInit(hit, nullptr, nullptr, &GLOBAL_camera_mainCamera.eyePosition, &GLOBAL_camera_mainCamera.Z, nullptr, 0.0);
    hit->normal = GLOBAL_camera_mainCamera.Z;
    hit->X = GLOBAL_camera_mainCamera.X;
    hit->Y = GLOBAL_camera_mainCamera.Y;
    hit->Z = GLOBAL_camera_mainCamera.Z;
    hit->flags |= HIT_NORMAL | HIT_SHADINGFRAME;

    vectorCopy(newNode->m_hit.normal, newNode->m_normal);
    newNode->m_G = 1.0;

    // outDir's not filled in

    newNode->m_pdfFromPrev = 1.0; /* Differential eye area cancels
				     out with computing the flux */

    newNode->m_pdfFromNext = 0.0; /* Eye cannot be hit accidently,
				   this pdf is never used */

    newNode->m_useBsdf = nullptr;
    newNode->m_inBsdf = nullptr;
    newNode->m_outBsdf = nullptr;

    // Component propagation
    newNode->m_accUsedComponents = NO_COMPONENTS; // Eye had no accumulated comps.

    newNode->accumulatedRussianRouletteFactors = 1.0; // No russian roulette

    return true;
}

double CEyeSampler::evalPDF(SimpleRaytracingPathNode */*thisNode*/,
                            SimpleRaytracingPathNode */*newNode*/, BSDF_FLAGS /*flags*/,
                            double * /*probabilityDensityFunction*/, double * /*probabilityDensityFunctionRR*/) {
    return 1.0;
}


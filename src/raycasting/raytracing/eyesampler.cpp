#include "common/error.h"
#include "shared/camera.h"
#include "scene/scene.h"
#include "raycasting/raytracing/eyesampler.h"

bool CEyeSampler::Sample(CPathNode *prevNode, CPathNode *thisNode,
                         CPathNode *newNode, double /*x_1*/, double /*x_2*/,
                         bool /* doRR */, BSDFFLAGS /* flags */) {
    if ( prevNode != nullptr || thisNode != nullptr ) {
        logWarning("CEyeSampler::Sample", "Not first node in path ?!");
    }

    // Just fill in newNode with camera data. Appropiate pdf fields are
    // are set to 1

    newNode->m_depth = 0;  // We expect this to be the first node in an eye path
    newNode->m_rayType = Stops;

    // Choose eye : N/A
    // Choose point on eye : N/A

    // Fake a hit record

    RayHit *hit = &newNode->m_hit;

    InitHit(hit, nullptr, nullptr, &GLOBAL_camera_mainCamera.eyep, &GLOBAL_camera_mainCamera.Z, nullptr, 0.);
    hit->normal = GLOBAL_camera_mainCamera.Z;
    hit->X = GLOBAL_camera_mainCamera.X;
    hit->Y = GLOBAL_camera_mainCamera.Y;
    hit->Z = GLOBAL_camera_mainCamera.Z;
    hit->flags |= HIT_NORMAL | HIT_SHADINGFRAME;

    VECTORCOPY(newNode->m_hit.normal, newNode->m_normal);
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

    newNode->m_rracc = 1.0; // No russian roulette

    return true;
}

double CEyeSampler::EvalPDF(CPathNode */*thisNode*/,
                            CPathNode */*newNode*/, BSDFFLAGS /*flags*/,
                            double * /*pdf*/, double * /*pdfRR*/) {
    return 1.0;
}


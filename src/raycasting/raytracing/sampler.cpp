#include "material/hit.h"
#include "scene/scene.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/sampler.h"

bool
CSampler::SampleTransfer(CPathNode *thisNode,
                              CPathNode *newNode, Vector3D *dir,
                              double pdfDir) {
    Ray ray;
    RayHit *hit;

    ray.pos = thisNode->m_hit.point;
    ray.dir = *dir;

    // Fill in depth

    newNode->m_depth = thisNode->m_depth + 1;
    newNode->m_rayType = Stops;

    hit = findRayIntersection(&ray, thisNode->m_hit.patch,
                              newNode->m_inBsdf, &newNode->m_hit);

    if ( !hit ) {
        if ( GLOBAL_scene_background ) {
            // Fill in pathnode for background
            hitInit(&(newNode->m_hit), GLOBAL_scene_background->bkgPatch, nullptr, nullptr, dir, nullptr, HUGE);
            newNode->m_inDirT = *dir;
            newNode->m_inDirF = -(*dir);
            newNode->m_pdfFromPrev = pdfDir;
            newNode->m_G = fabs(vectorDotProduct(thisNode->m_hit.normal, newNode->m_inDirT));
            newNode->m_inBsdf = thisNode->m_outBsdf;
            newNode->m_useBsdf = nullptr;
            newNode->m_outBsdf = nullptr;
            newNode->m_rayType = Environment;
            newNode->m_hit.flags |= HIT_FRONT;

            return true;
        }

            // if no background is present
        else {
            return (false);
        }
    }

    if ( hit->flags & HIT_BACK ) {
        // Back hit, invert normal (only happens when newNode->m_inBsdf != nullptr
        vectorScale(-1, newNode->m_hit.normal, newNode->m_hit.normal);
    }

    vectorCopy(ray.dir, newNode->m_inDirT);
    vectorScale(-1, newNode->m_inDirT, newNode->m_inDirF);

    // Check for shading normal vs. geometric normal errors

    if ( vectorDotProduct(newNode->m_hit.normal, newNode->m_inDirF) < 0 ) {
        // Error("Sample", "Shading normal anomaly");
        return false;
    }


    // Compute geometry factor cos(a)*cos(b)/r^2
    double cosa, cosb, dist2;
    Vector3D tmpVec;

    cosa = fabs(vectorDotProduct(thisNode->m_hit.normal,
                                 newNode->m_inDirT));
    cosb = fabs(vectorDotProduct(newNode->m_hit.normal,
                                 newNode->m_inDirT));
    vectorSubtract(newNode->m_hit.point, thisNode->m_hit.point, tmpVec);
    dist2 = vectorNorm2(tmpVec);

    if ( dist2 < EPSILON ) {
        /* Next node is useless, gives rise to
           numeric errors (Inf) */
        return false;
    }

    newNode->m_G = cosa * cosb / dist2; // Integrate over area !

    // Fill in probability

    newNode->m_pdfFromPrev = pdfDir * cosb / dist2;

    return true; // Transfer succeeded
}

void
CSurfaceSampler::DetermineRayType(CPathNode *thisNode,
                                       CPathNode *newNode, Vector3D *dir) {
    double cosThisPatch = vectorDotProduct(*dir, thisNode->m_normal);

    if ( cosThisPatch < 0 ) {
        // Refraction !
        if ( thisNode->m_hit.flags & HIT_BACK ) {
            thisNode->m_rayType = Leaves;
        } else {
            thisNode->m_rayType = Enters;
        }

        newNode->m_inBsdf = thisNode->m_outBsdf;
    } else {
        // Reflection
        thisNode->m_rayType = Reflects;
        newNode->m_inBsdf = thisNode->m_inBsdf; // staying in same medium
    }
}

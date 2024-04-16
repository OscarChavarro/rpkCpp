#include "material/RayHit.h"
#include "common/mymath.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/sampler.h"

/**
sample transfer generates a new point on a surface by ray tracing
given a certain direction and the pdf for that direction
The medium the ray is traveling through must be known and
is given by newNode->m_inBsdf.
The function fills in newNode: hit, normal, directions, pdf (area)
         geometry factor and depth  (rayType is un-touched)
It returns false if no point was found when tracing a ray
or if a shading normal anomaly occurs
*/
bool
Sampler::sampleTransfer(
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    Vector3D *dir,
    double pdfDir)
{
    Ray ray;
    RayHit *hit;

    ray.pos = thisNode->m_hit.point;
    ray.dir = *dir;

    // Fill in depth
    newNode->m_depth = thisNode->m_depth + 1;
    newNode->m_rayType = STOPS;
    hit = findRayIntersection(
        sceneVoxelGrid,
        &ray,
        thisNode->m_hit.patch,
        newNode->m_inBsdf,
        &newNode->m_hit);

    if ( !hit ) {
        if ( sceneBackground ) {
            // Fill in path node for background
            hitInit(&(newNode->m_hit), sceneBackground->bkgPatch, nullptr, nullptr, dir, nullptr, HUGE);
            newNode->m_inDirT = *dir;
            newNode->m_inDirF = -(*dir);
            newNode->m_pdfFromPrev = pdfDir;
            newNode->m_G = std::fabs(vectorDotProduct(thisNode->m_hit.normal, newNode->m_inDirT));
            newNode->m_inBsdf = thisNode->m_outBsdf;
            newNode->m_useBsdf = nullptr;
            newNode->m_outBsdf = nullptr;
            newNode->m_rayType = ENVIRONMENT;
            newNode->m_hit.flags |= HIT_FRONT;

            return true;
        } else {
            // if no background is present
            return false;
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
    double dist2;
    Vector3D tmpVec;

    double cosA = std::fabs(vectorDotProduct(thisNode->m_hit.normal, newNode->m_inDirT));
    double cosB = std::fabs(vectorDotProduct(newNode->m_hit.normal, newNode->m_inDirT));
    vectorSubtract(newNode->m_hit.point, thisNode->m_hit.point, tmpVec);
    dist2 = vectorNorm2(tmpVec);

    if ( dist2 < EPSILON ) {
        // Next node is useless, gives rise to numeric errors (Inf)
        return false;
    }

    newNode->m_G = cosA * cosB / dist2; // Integrate over area !

    // Fill in probability
    newNode->m_pdfFromPrev = pdfDir * cosB / dist2;

    return true; // Transfer succeeded
}

void
CSurfaceSampler::DetermineRayType(
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    Vector3D *dir)
{
    double cosThisPatch = vectorDotProduct(*dir, thisNode->m_normal);

    if ( cosThisPatch < 0 ) {
        // Refraction !
        if ( thisNode->m_hit.flags & HIT_BACK ) {
            thisNode->m_rayType = LEAVES;
        } else {
            thisNode->m_rayType = ENTERS;
        }

        newNode->m_inBsdf = thisNode->m_outBsdf;
    } else {
        // Reflection
        thisNode->m_rayType = REFLECTS;
        newNode->m_inBsdf = thisNode->m_inBsdf; // staying in same medium
    }
}

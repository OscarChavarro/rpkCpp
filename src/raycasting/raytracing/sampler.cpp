#include "material/RayHit.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/sampler.h"

#ifdef RAYTRACING_ENABLED

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
    const RayHit *hit;

    ray.pos = thisNode->m_hit.getPoint();
    ray.dir = *dir;

    // Fill in depth
    newNode->m_depth = thisNode->m_depth + 1;
    newNode->m_rayType = STOPS;
    hit = findRayIntersection(
        sceneVoxelGrid,
        &ray,
        thisNode->m_hit.getPatch(),
        newNode->m_inBsdf,
        &newNode->m_hit);

    if ( !hit ) {
        if ( sceneBackground ) {
            // Fill in path node for background
            newNode->m_hit.init(sceneBackground->bkgPatch, nullptr, dir, nullptr);
            newNode->m_inDirT = *dir;
            newNode->m_inDirF.set(dir->x, dir->y, dir->z);
            newNode->m_pdfFromPrev = pdfDir;
            newNode->m_G = java::Math::abs(thisNode->m_hit.getNormal().dotProduct(newNode->m_inDirT));
            newNode->m_inBsdf = thisNode->m_outBsdf;
            newNode->m_useBsdf = nullptr;
            newNode->m_outBsdf = nullptr;
            newNode->m_rayType = ENVIRONMENT;
            unsigned int newFlags = newNode->m_hit.getFlags() | RayHitFlag::FRONT;
            newNode->m_hit.setFlags(newFlags);

            return true;
        } else {
            // if no background is present
            return false;
        }
    }

    if ( hit->getFlags() & RayHitFlag::BACK ) {
        // Back hit, invert normal (only happens when newNode->m_inBsdf != nullptr
        Vector3D normal;
        normal.scaledCopy(-1, newNode->m_hit.getNormal());
        newNode->m_hit.setNormal(&normal);
    }

    newNode->m_inDirT.copy(ray.dir);
    newNode->m_inDirF.scaledCopy(-1, newNode->m_inDirT);

    // Check for shading normal vs. geometric normal errors
    if ( newNode->m_hit.getNormal().dotProduct(newNode->m_inDirF) < 0 ) {
        // Shading normal anomaly
        return false;
    }

    // Compute geometry factor cos(a)*cos(b)/r^2
    double dist2;
    Vector3D tmpVec;

    double cosA = java::Math::abs(thisNode->m_hit.getNormal().dotProduct(newNode->m_inDirT));
    double cosB = java::Math::abs(newNode->m_hit.getNormal().dotProduct(newNode->m_inDirT));
    tmpVec.subtraction(newNode->m_hit.getPoint(), thisNode->m_hit.getPoint());
    dist2 = tmpVec.norm2();

    if ( dist2 < Numeric::EPSILON ) {
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
    const Vector3D *dir)
{
    double cosThisPatch = dir->dotProduct(thisNode->m_normal);

    if ( cosThisPatch < 0 ) {
        // Refraction !
        if ( thisNode->m_hit.getFlags() & RayHitFlag::BACK ) {
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

#endif

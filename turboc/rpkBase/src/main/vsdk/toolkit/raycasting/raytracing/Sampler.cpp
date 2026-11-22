#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/raycasting/common/Raytools.h"
#include "vsdk/toolkit/raycasting/raytracing/Sampler.h"

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
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneSceneQueryWorkspace,
    Background *sceneBackground,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    Vector3Df *dir,
    double pdfDir)
{
    RpkRay ray;
    ray.position = thisNode->m_hit.getPoint();
    ray.direction = *dir;

    // Fill in depth
    newNode->m_depth = thisNode->m_depth + 1;
    newNode->m_rayType = STOPS;
    const RayHit *hit = RayTools::findRayIntersection(
        sceneQuery,
        sceneSceneQueryWorkspace,
        &ray,
        thisNode->m_hit.getPatch(),
        newNode->m_inBsdf,
        &newNode->m_hit);

    if ( hit == NULL ) {
        if ( sceneBackground != NULL ) {
            // Fill in path node for background
            newNode->m_hit.init(sceneBackground->bkgPatch, NULL, dir, NULL);
            newNode->m_inDirT = *dir;
            newNode->m_inDirF = *dir;
            newNode->m_pdfFromPrev = pdfDir;
            newNode->m_G = Math::abs(thisNode->m_hit.getNormal().dotProduct(newNode->m_inDirT));
            newNode->m_inBsdf = thisNode->m_outBsdf;
            newNode->m_useBsdf = NULL;
            newNode->m_outBsdf = NULL;
            newNode->m_rayType = ENVIRONMENT;
            unsigned int newFlags = newNode->m_hit.getFlags() | FRONT;
            newNode->m_hit.setFlags(newFlags);

            return true;
        } else {
            // if no background is present
            return false;
        }
    }

    if ( hit->getFlags() & BACK ) {
        // Back hit, invert normal (only happens when newNode->m_inBsdf != NULL
        Vector3Df normal;
        normal = (newNode->m_hit.getNormal()).multiply(-1);
        newNode->m_hit.setNormal(&normal);
    }

    newNode->m_inDirT = ray.direction;
    newNode->m_inDirF = newNode->m_inDirT.multiply(-1);

    // Check for shading normal vs. geometric normal errors
    if ( newNode->m_hit.getNormal().dotProduct(newNode->m_inDirF) < 0 ) {
        // Shading normal anomaly
        return false;
    }

    // Compute geometry factor cos(a)*cos(b)/r^2
    double dist2;
    Vector3Df tmpVec;

    double cosA = Math::abs(thisNode->m_hit.getNormal().dotProduct(newNode->m_inDirT));
    double cosB = Math::abs(newNode->m_hit.getNormal().dotProduct(newNode->m_inDirT));
    tmpVec = (newNode->m_hit.getPoint()).subtract(thisNode->m_hit.getPoint());
    dist2 = tmpVec.lengthSquared();

    if ( dist2 < VSDK::EPSILON ) {
        // Next node is useless, gives rise to numeric errors (Inf)
        return false;
    }

    newNode->m_G = cosA * cosB / dist2; // Integrate over area !

    // Fill in probability
    newNode->m_pdfFromPrev = pdfDir * cosB / dist2;

    return true; // Transfer succeeded
}

void
SurfaceSampler::DetermineRayType(
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    const Vector3Df *dir)
{
    double cosThisPatch = dir->dotProduct(thisNode->m_normal);

    if ( cosThisPatch < 0 ) {
        // Refraction !
        if ( thisNode->m_hit.getFlags() & BACK ) {
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

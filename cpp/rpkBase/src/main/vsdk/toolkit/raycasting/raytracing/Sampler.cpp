#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"
#include "java/util/ArrayList.txx"
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
    Vector3Dd *dir,
    double pdfDir)
{
    Ray ray;
    ray.setOrigin(thisNode->m_hit.getPoint());
    ray.setDirection(*dir);

    // Fill in depth
    newNode->m_depth = thisNode->m_depth + 1;
    newNode->m_rayType = PathRayType::STOPS;
    const RpkRayHit *hit = RayTools::findRayIntersection(
        sceneQuery,
        sceneSceneQueryWorkspace,
        &ray,
        thisNode->m_hit.getPatch(),
        newNode->m_inBsdf,
        &newNode->m_hit);

    if ( hit == nullptr ) {
        if ( sceneBackground != nullptr ) {
            // Fill in path node for background
            newNode->m_hit.init(sceneBackground->bkgPatch, nullptr, dir, nullptr);
            newNode->m_inDirT = *dir;
            newNode->m_inDirF = *dir;
            newNode->m_pdfFromPrev = pdfDir;
            newNode->m_G = java::Math::abs(thisNode->m_hit.getNormal().dotProduct(newNode->m_inDirT));
            newNode->m_inBsdf = thisNode->m_outBsdf;
            newNode->m_useBsdf = nullptr;
            newNode->m_outBsdf = nullptr;
            newNode->m_rayType = PathRayType::ENVIRONMENT;
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
        Vector3Dd normal;
        normal = (newNode->m_hit.getNormal()).multiply(-1);
        newNode->m_hit.setNormal(&normal);
    }

    newNode->m_inDirT = ray.getDirection();
    newNode->m_inDirF = newNode->m_inDirT.multiply(-1);

    // Check for shading normal vs. geometric normal errors
    if ( newNode->m_hit.getNormal().dotProduct(newNode->m_inDirF) < 0 ) {
        // Shading normal anomaly
        return false;
    }

    // Compute geometry factor cos(a)*cos(b)/r^2
    double dist2;
    Vector3Dd tmpVec;

    double cosA = java::Math::abs(thisNode->m_hit.getNormal().dotProduct(newNode->m_inDirT));
    double cosB = java::Math::abs(newNode->m_hit.getNormal().dotProduct(newNode->m_inDirT));
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
    const Vector3Dd *dir)
{
    double cosThisPatch = dir->dotProduct(thisNode->m_normal);

    if ( cosThisPatch < 0 ) {
        // Refraction !
        if ( thisNode->m_hit.getFlags() & RayHitFlag::BACK ) {
            thisNode->m_rayType = PathRayType::LEAVES;
        } else {
            thisNode->m_rayType = PathRayType::ENTERS;
        }

        newNode->m_inBsdf = thisNode->m_outBsdf;
    } else {
        // Reflection
        thisNode->m_rayType = PathRayType::REFLECTS;
        newNode->m_inBsdf = thisNode->m_inBsdf; // staying in same medium
    }
}

ColorRgb
SurfaceSampler::DoBsdfEval(
    const PhongBidirectionalScatteringDistributionFunction *bsdf,
    RpkRayHit *hit,
    const PhongBidirectionalScatteringDistributionFunction *inBsdf,
    const PhongBidirectionalScatteringDistributionFunction *outBsdf,
    const Vector3Dd *in,
    const Vector3Dd *out,
    char flags,
    BsdfComp *bsdfComp) const
{
    bool ok = false;
    const ShadingContext context = hit->shadingContext(&ok);

    if ( m_computeBsdfComponents ) {
        if ( bsdf == nullptr ) {
            return ColorRgb(0.0, 0.0, 0.0);
        }
        java::ArrayList<ColorRgb> componentArray(BsdfComponentInfo::BSDF_COMPONENTS);
        for ( int i = 0; i < BsdfComponentInfo::BSDF_COMPONENTS; i++ ) {
            componentArray.add(ColorRgb(0.0, 0.0, 0.0));
        }
        const ColorRgb radiance = bsdf->bsdfEvalComponents(
            context, inBsdf, outBsdf, in, out, flags, componentArray.data());
        for ( int i = 0; i < BsdfComponentInfo::BSDF_COMPONENTS; i++ ) {
            (*bsdfComp)[i] = componentArray[i];
        }
        return radiance;
    }

    bsdfComp->Clear();
    if ( bsdf == nullptr ) {
        return ColorRgb(0.0, 0.0, 0.0);
    }
    return bsdf->evaluate(context, inBsdf, outBsdf, in, out, flags);
}

#endif

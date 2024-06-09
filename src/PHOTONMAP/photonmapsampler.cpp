/**
This is a hack to get fresnel factors for perfect specular reflection and refraction
*/

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "PHOTONMAP/photonmapsampler.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "common/error.h"
#include "raycasting/common/raytools.h"

CPhotonMapSampler::CPhotonMapSampler() {
    m_photonMap = nullptr;
}

/**
Returns true a component was chosen, false if absorbed
*/
bool
CPhotonMapSampler::chooseComponent(
    char flags1,
    char flags2,
    const PhongBidirectionalScatteringDistributionFunction *bsdf,
    RayHit *hit,
    bool doRR,
    double *x,
    float *probabilityDensityFunction,
    bool *chose1)
{
    ColorRgb color;
    float power1;
    float power2;
    float totalPower;

    // Choose between flags1 or flags2 scattering
    color.clear();

    if ( bsdf != nullptr ) {
        color = bsdf->splitBsdfScatteredPower(hit, flags1);
    }
    power1 = color.average();

    color.clear();
    if ( bsdf != nullptr ) {
        color = bsdf->splitBsdfScatteredPower(hit, flags2);
    }
    power2 = color.average();

    totalPower = power1 + power2;

    if ( totalPower < Numeric::EPSILON ) {
        // No significant scattering
        return false;
    }

    // Account for russian roulette
    if ( !doRR ) {
        power1 /= totalPower;
        power2 /= totalPower;
        totalPower = 1.0;
    }

    // Use x for scattering choice
    if ( *x < power1 ) {
        *chose1 = true;
        *probabilityDensityFunction = power1;
        *x = *x / power1;
    } else if ( *x < totalPower ) {
        *chose1 = false;
        *probabilityDensityFunction = power2;
        *x = (*x - power1) / power2;
    } else {
        // Absorbed
        return false;
    }

    return true;
}

bool
CPhotonMapSampler::sample(
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
    const PhongBidirectionalScatteringDistributionFunction *bsdf = thisNode->m_useBsdf;
    bool sChosen;
    float pdfChoice;
    char sFlagMask;

    const char sFLAGS = BSDF_SPECULAR_COMPONENT;
    const char gdFLAGS = BRDF_GLOSSY_COMPONENT | BRDF_DIFFUSE_COMPONENT;

    // Choose between S or D|G scattering
    // Hack to get separate specular en fresnel ok...
    if ( flags == BRDF_SPECULAR_COMPONENT ) {
        // Specular transmission can result in reflection
        sFlagMask = sFLAGS;
    } else {
        sFlagMask = flags;
    }
    if ( !chooseComponent(
        (char)(sFLAGS & sFlagMask),
        (char)(gdFLAGS & flags),
        bsdf,
        &thisNode->m_hit,
        doRR,
        &x2,
        &pdfChoice,
        &sChosen) ) {
        return false; // Absorbed
    }

    // Get a sampled direction
    bool ok;

    if ( sChosen ) {
        ok = fresnelSample(sceneVoxelGrid, sceneBackground, prevNode, thisNode, newNode, x2, flags);
    } else {
        flags = (char)(gdFLAGS & flags);
        ok = gdSample(camera, sceneVoxelGrid, sceneBackground, prevNode, thisNode, newNode, x1, x2, flags);
    }

    if ( ok ) {
        // Adjust probabilityDensityFunction with s vs gd choice
        newNode->m_pdfFromPrev *= pdfChoice;

        // Component propagation
        newNode->m_accUsedComponents = (char)(thisNode->m_accUsedComponents | thisNode->m_usedComponents);
    }

    return ok;
}

/** The Fresnel sampler works as follows:
1. Index of refractions are taken
2. Reflectance and Transmittance values are taken. Normally one of the two
   would be zero.
2b. Complex index of refraction, converted into geometric iof
3. Perfect reflected and refracted (if necessary) directions are computed
4. cosines and fresnel formulas are computed
5. reflection or refraction is chosen
6. fresnel reflection/refraction multiplied by appropriate scattering powers
7. node filled in.
*/
static RefractionIndex
bsdfGeometricIOR(const PhongBidirectionalScatteringDistributionFunction *bsdf) {
    RefractionIndex nc{};

    if ( bsdf == nullptr ) {
        nc.set(1.0, 0.0); // Vacuum
    } else {
        bsdf->indexOfRefraction(&nc);
    }

    // Convert to geometric IOR if necessary
    if ( nc.getNi() > Numeric::EPSILON ) {
        nc.set(nc.complexToGeometricRefractionIndex(), 0.0);
    }

    return nc;
}

static bool
chooseFresnelDirection(
    SimpleRaytracingPathNode *thisNode,
    char flags,
    double x2,
    Vector3D *dir,
    double *pdfDir,
    ColorRgb *scatteringColor,
    bool *doCosInverse)
{
    // Index of refractions are taken
    RefractionIndex nc_in = bsdfGeometricIOR(thisNode->m_inBsdf);
    RefractionIndex nc_out = bsdfGeometricIOR(thisNode->m_outBsdf);

    // Reflectance and Transmittance values are taken. Normally one of the two
    // would be zero
    const PhongBidirectionalScatteringDistributionFunction *bsdf = thisNode->m_useBsdf;
    ColorRgb reflectance;
    reflectance.clear();
    if ( bsdf != nullptr ) {
        reflectance = bsdf->splitBsdfScatteredPower(&thisNode->m_hit, SPECULAR_COMPONENT);
    }

    ColorRgb transmittance;
    transmittance.clear();
    if ( bsdf != nullptr ) {
        transmittance = bsdf->splitBsdfScatteredPower(&thisNode->m_hit, SPECULAR_COMPONENT);
    }

    bool reflective = (reflectance.average() > Numeric::EPSILON);
    bool trans = (transmittance.average() > Numeric::EPSILON);

    if ( reflective && trans ) {
        logError("FresnelFactor",
                 "Cannot deal with simultaneous reflective & transit materials");
        return false;
    }

    // Fresnel reflection factor is computed.
    float cosI;
    float cost;
    float F;  // Fresnel reflection. (Refraction = 1 - T)
    bool tir; // total internal reflection
    Vector3D reflectedDir;
    Vector3D refractedDir;

    if ( reflective ) {
        if ( flags & BRDF_SPECULAR_COMPONENT ) {
            // Hack !?
            F = 1.0;
            reflectedDir = idealReflectedDirection(&thisNode->m_inDirT,
                                                   &thisNode->m_normal);
            cosI = thisNode->m_normal.dotProduct(thisNode->m_inDirF);
            if ( cosI < 0 ) {
                logError("fresnelSample", "cosI < 0");
            }
        } else {
            F = 0;
        }
    } else {
        refractedDir = idealRefractedDirection(&thisNode->m_inDirT,
                                               &thisNode->m_normal,
                                               nc_in, nc_out, &tir);

        if ( !tir ) {
            reflectedDir = idealReflectedDirection(&thisNode->m_inDirT,
                                                   &thisNode->m_normal);
        }

        // 4. Cosines and fresnel formulas are computed
        cosI = thisNode->m_normal.dotProduct(thisNode->m_inDirF);

        if ( cosI < 0 ) {
            logError("fresnelSample", "cosI < 0");
        }

        if ( !tir ) {
            cost = -thisNode->m_normal.dotProduct(refractedDir);

            if ( cost < 0 ) {
                logError("fresnelSample", "cost < 0");
            }

            float rParallel;
            float rPerpendicular;
            float nt = nc_out.getNr();
            float ni = nc_in.getNr();

            rParallel = (nt * cosI - ni * cost) / (nt * cosI + ni * cost);
            rPerpendicular = (ni * cosI - nt * cost) / (ni * cosI + nt * cost);

            F = 0.5f * (rParallel * rParallel + rPerpendicular * rPerpendicular);
        } else {
            F = 0; // All in refracted dir, which == reflected dir
        }
    }

    float T = 1.0f - F;
    bool reflected;

    // Choose reflection or refraction
    float sum = 0.0;

    if ( flags & BTDF_SPECULAR_COMPONENT ) {
        sum += T;
    } else {
        T = 0;
    }

    if ( flags & BRDF_SPECULAR_COMPONENT ) {
        sum += F;
    } else {
        F = 0;
    }

    if ( sum < Numeric::EPSILON ) {
        return false;
    }

    if ( x2 < T / sum ) {
        reflected = false;
        *dir = refractedDir;
        *pdfDir = T / sum;
    } else {
        reflected = true;
        *dir = reflectedDir;
        *pdfDir = F / sum;
    }

    // Compute bsdf evaluation here and determine if we
    // still need to divide by the cosine to get 'real'
    // specular transmission
    if ( reflected ) {
        if ( reflective ) {
            scatteringColor->scaledCopy(F, reflectance);
            *doCosInverse = false;
        } else {
            scatteringColor->scaledCopy(F, transmittance);
            *doCosInverse = true;
        }
    } else {
        scatteringColor->scaledCopy(T, transmittance);
        *doCosInverse = true;
    }

    return true;
}

bool
CPhotonMapSampler::fresnelSample(
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    const SimpleRaytracingPathNode *prevNode,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x2,
    char flags)
{
    Vector3D dir;
    double pdfDir;
    bool doCosInverse;
    ColorRgb scatteringColor;

    if ( !chooseFresnelDirection(thisNode, flags, x2, &dir, &pdfDir,
                                 &scatteringColor, &doCosInverse) ) {
        return false;
    }

    // Reflection Type, changes thisNode->m_rayType and newNode->m_inBsdf
    DetermineRayType(thisNode, newNode, &dir);

    // Transfer
    if ( !sampleTransfer(sceneVoxelGrid, sceneBackground, thisNode, newNode, &dir, pdfDir) ) {
        thisNode->m_rayType = PathRayType::STOPS;
        return false;
    }

    // Fill in bsdf evaluation. This is just reflectance or transmittance
    // given by ChooseFresnelDirection, but may be divided by a cosine
    // -- No bsdf components yet here !!
    if ( doCosInverse ) {
        float cosB = java::Math::abs(newNode->m_hit.getNormal().dotProduct(newNode->m_inDirT));
        thisNode->m_bsdfEval.scaleInverse(cosB, scatteringColor);
    } else {
        thisNode->m_bsdfEval = scatteringColor;
    }

    // Fill in probability for previous node
    if ( m_computeFromNextPdf && prevNode ) {
        logWarning("FresnelSampler", "FromNextPdf not supported");
    }

    // Component propagation
    if ( thisNode->m_rayType == PathRayType::REFLECTS ) {
        thisNode->m_usedComponents = BRDF_SPECULAR_COMPONENT;
    } else {
        thisNode->m_usedComponents = BTDF_SPECULAR_COMPONENT;
    }

    return true; // Node filled in
}

bool
CPhotonMapSampler::gdSample(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *prevNode,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    char flags)
{
    bool ok;

    // Sample G|D and use m_photonMap for importance sampling if possible.
    if ( m_photonMap == nullptr ) {
        // We can just use standard bsdf sampling
        ok = CBsdfSampler::sample(
            camera, sceneVoxelGrid, sceneBackground, prevNode, thisNode, newNode, x1, x2, false, flags);
        thisNode->m_usedComponents = flags;
        return ok;
    }

    // -- Currently NEVER reached!

    // Choose G or D
    const PhongBidirectionalScatteringDistributionFunction *bsdf = thisNode->m_useBsdf;
    bool dChosen;
    float pdfChoice;

    // Choose between D or G scattering
    if ( !chooseComponent(BRDF_DIFFUSE_COMPONENT & flags,
                          BRDF_GLOSSY_COMPONENT & flags,
                          bsdf,
                          &thisNode->m_hit,
                          false,
                          &x1,
                          &pdfChoice,
                          &dChosen) ) {
        return false;
    }

    // Importance sampling using photon map x_1 & x_2 get transformed

    CoordinateSystem coord;
    float glossy_exponent;

    if ( dChosen ) {
        coord.setFromZAxis(&thisNode->m_normal);
        glossy_exponent = 1;
        flags = BRDF_DIFFUSE_COMPONENT;
    } else {
        flags = BRDF_GLOSSY_COMPONENT;

        logError("CPhotonMapSampler::gdSample", "Not done yet");
        return false;
    }

    double photonMapPdf = m_photonMap->sample(thisNode->m_hit.getPoint(), &x1, &x2, &coord, flags, glossy_exponent);

    // Do real sampling
    ok = CBsdfSampler::sample(
        camera,
        sceneVoxelGrid,
        sceneBackground,
        prevNode,
        thisNode,
        newNode,
        x1,
        x2,
        false,
        flags);

    // Adjust probabilityDensityFunction
    if ( ok ) {
        newNode->m_pdfFromPrev *= pdfChoice * photonMapPdf;
        thisNode->m_usedComponents = flags;
    }

    return ok;
}

#endif

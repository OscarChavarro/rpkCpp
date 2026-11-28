#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"

/**
This is a hack to get fresnel factors for perfect specular reflection and refraction
*/


#ifdef RAYTRACING_ENABLED
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapSampler.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/raycasting/common/Raytools.h"

PhotonMapSampler::PhotonMapSampler() {
    m_photonMap = NULL;
}

/**
Returns true a component was chosen, false if absorbed
*/
bool
PhotonMapSampler::chooseComponent(
    char flags1,
    char flags2,
    const PhongBidirScattDistFunc *bsdf,
    RpkRayHit *hit,
    bool doRR,
    double *x,
    double *probabilityDensityFunction,
    bool *chose1)
{
    ColorRgb color;
    double power1;
    double power2;
    double totalPower;

    // Choose between flags1 or flags2 scattering
    color = ColorRgb(0.0, 0.0, 0.0);

    if ( bsdf != NULL ) {
        color = bsdf->splitBsdfScatteredPower(hit->shadingContext(NULL), flags1);
    }
    power1 = ColorRgbMutable(color).average();

    color = ColorRgb(0.0, 0.0, 0.0);
    if ( bsdf != NULL ) {
        color = bsdf->splitBsdfScatteredPower(hit->shadingContext(NULL), flags2);
    }
    power2 = ColorRgbMutable(color).average();

    totalPower = power1 + power2;

    if ( totalPower < VSDK::EPSILON ) {
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
PhotonMapSampler::sample(
    Camera *camera,
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneSceneQueryWorkspace,
    Background *sceneBackground,
    SimpleRaytracingPathNode *prevNode,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    bool doRR,
    char flags)
{
    const PhongBidirScattDistFunc *bsdf = thisNode->m_useBsdf;
    bool sChosen;
    double pdfChoice;
    char sFlagMask;

    const char sFLAGS = BsdfComponentInfo::BSDF_SPECULAR_COMPONENT;
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
        ((char)(sFLAGS & sFlagMask)),
        ((char)(gdFLAGS & flags)),
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
        ok = fresnelSample(sceneQuery, sceneSceneQueryWorkspace, sceneBackground, prevNode, thisNode, newNode, x2, flags);
    } else {
        flags = ((char)(gdFLAGS & flags));
        ok = gdSample(camera, sceneQuery, sceneSceneQueryWorkspace, sceneBackground, prevNode, thisNode, newNode, x1, x2, flags);
    }

    if ( ok ) {
        // Adjust probabilityDensityFunction with s vs gd choice
        newNode->m_pdfFromPrev *= pdfChoice;

        // Component propagation
        newNode->m_accUsedComponents = ((char)(thisNode->m_accUsedComponents | thisNode->m_usedComponents));
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
RefractionIndex
PhotonMapSampler::bsdfGeometricIOR(const PhongBidirScattDistFunc *bsdf) {
    RefractionIndex nc = RefractionIndex();

    if ( bsdf == NULL ) {
        nc.set(1.0, 0.0); // Vacuum
    } else {
        bsdf->indexOfRefraction(&nc);
    }

    // Convert to geometric IOR if necessary
    if ( nc.getNi() > VSDK::EPSILON ) {
        nc.set(nc.cmplxTGmtrcRfrctIndx(), 0.0);
    }

    return nc;
}

bool
PhotonMapSampler::chooseFresnelDirection(
    SimpleRaytracingPathNode *thisNode,
    char flags,
    double x2,
    Vector3Dd *dir,
    double *pdfDir,
    ColorRgb *scatteringColor,
    bool *doCosInverse)
{
    // Index of refractions are taken
    RefractionIndex nc_in = PhotonMapSampler::bsdfGeometricIOR(thisNode->m_inBsdf);
    RefractionIndex nc_out = PhotonMapSampler::bsdfGeometricIOR(thisNode->m_outBsdf);

    // Reflectance and Transmittance values are taken. Normally one of the two
    // would be zero
    const PhongBidirScattDistFunc *bsdf = thisNode->m_useBsdf;
    ColorRgb reflectance;
    reflectance = ColorRgb(0.0, 0.0, 0.0);
    if ( bsdf != NULL ) {
        reflectance = bsdf->splitBsdfScatteredPower(thisNode->m_hit.shadingContext(NULL), SPECULAR_COMPONENT);
    }

    ColorRgb transmittance;
    transmittance = ColorRgb(0.0, 0.0, 0.0);
    if ( bsdf != NULL ) {
        transmittance = bsdf->splitBsdfScatteredPower(thisNode->m_hit.shadingContext(NULL), SPECULAR_COMPONENT);
    }

    bool reflective = (ColorRgbMutable(reflectance).average() > VSDK::EPSILON);
    bool trans = (ColorRgbMutable(transmittance).average() > VSDK::EPSILON);

    if ( reflective && trans ) {
        Logger::reportMessage("PhotonMapSampler", Logger::ERROR, "FresnelFactor", "Cannot deal with simultaneous reflective & transit materials");
        return false;
    }

    // Fresnel reflection factor is computed.
    double cosI;
    double cost;
    double F;  // Fresnel reflection. (Refraction = 1 - T)
    bool tir; // total internal reflection
    Vector3Dd reflectedDir;
    Vector3Dd refractedDir;

    if ( reflective ) {
        if ( flags & BRDF_SPECULAR_COMPONENT ) {
            // Hack !?
            F = 1.0;
            reflectedDir = Xxdf::idealReflectedDirection(&thisNode->m_inDirT,
                                                   &thisNode->m_normal);
            cosI = thisNode->m_normal.dotProduct(thisNode->m_inDirF);
            if ( cosI < 0 ) {
                Logger::reportMessage("PhotonMapSampler", Logger::ERROR, "fresnelSample", "cosI < 0");
            }
        } else {
            F = 0;
        }
    } else {
        refractedDir = Xxdf::idealRefractedDirection(&thisNode->m_inDirT,
                                               &thisNode->m_normal,
                                               nc_in, nc_out, &tir);

        if ( !tir ) {
            reflectedDir = Xxdf::idealReflectedDirection(&thisNode->m_inDirT,
                                                   &thisNode->m_normal);
        }

        // 4. Cosines and fresnel formulas are computed
        cosI = thisNode->m_normal.dotProduct(thisNode->m_inDirF);

        if ( cosI < 0 ) {
            Logger::reportMessage("PhotonMapSampler", Logger::ERROR, "fresnelSample", "cosI < 0");
        }

        if ( !tir ) {
            cost = -thisNode->m_normal.dotProduct(refractedDir);

            if ( cost < 0 ) {
                Logger::reportMessage("PhotonMapSampler", Logger::ERROR, "fresnelSample", "cost < 0");
            }

            double rParallel;
            double rPerpendicular;
            double nt = nc_out.getNr();
            double ni = nc_in.getNr();

            rParallel = (nt * cosI - ni * cost) / (nt * cosI + ni * cost);
            rPerpendicular = (ni * cosI - nt * cost) / (ni * cosI + nt * cost);

            F = 0.5 * (rParallel * rParallel + rPerpendicular * rPerpendicular);
        } else {
            F = 0; // All in refracted dir, which == reflected dir
        }
    }

    double T = 1.0 - F;
    bool reflected;

    // Choose reflection or refraction
    double sum = 0.0;

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

    if ( sum < VSDK::EPSILON ) {
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
            *scatteringColor = ColorRgb(
                F * reflectance.getR(),
                F * reflectance.getG(),
                F * reflectance.getB());
            *doCosInverse = false;
        } else {
            *scatteringColor = ColorRgb(
                F * transmittance.getR(),
                F * transmittance.getG(),
                F * transmittance.getB());
            *doCosInverse = true;
        }
    } else {
        *scatteringColor = ColorRgb(
            T * transmittance.getR(),
            T * transmittance.getG(),
            T * transmittance.getB());
        *doCosInverse = true;
    }

    return true;
}

bool
PhotonMapSampler::fresnelSample(
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneSceneQueryWorkspace,
    Background *sceneBackground,
    const SimpleRaytracingPathNode *prevNode,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x2,
    char flags)
{
    Vector3Dd dir;
    double pdfDir;
    bool doCosInverse;
    ColorRgb scatteringColor;

    if ( !PhotonMapSampler::chooseFresnelDirection(thisNode, flags, x2, &dir, &pdfDir,
                                 &scatteringColor, &doCosInverse) ) {
        return false;
    }

    // Reflection Type, changes thisNode->m_rayType and newNode->m_inBsdf
    DetermineRayType(thisNode, newNode, &dir);

    // Transfer
    if ( !sampleTransfer(sceneQuery, sceneSceneQueryWorkspace, sceneBackground, thisNode, newNode, &dir, pdfDir) ) {
        thisNode->m_rayType = STOPS;
        return false;
    }

    // Fill in bsdf evaluation. This is just reflectance or transmittance
    // given by ChooseFresnelDirection, but may be divided by a cosine
    // -- No bsdf components yet here !!
    if ( doCosInverse ) {
        double cosB = Math::abs(newNode->m_hit.getNormal().dotProduct(newNode->m_inDirT));
        double inv = (cosB < -VSDK::EPSILON || cosB > VSDK::EPSILON) ? 1.0 / cosB : 1.0;
        thisNode->m_bsdfEval = ColorRgb(
            inv * scatteringColor.getR(),
            inv * scatteringColor.getG(),
            inv * scatteringColor.getB());
    } else {
        thisNode->m_bsdfEval = scatteringColor;
    }

    // Fill in probability for previous node
    if ( m_computeFromNextPdf && prevNode ) {
        Logger::reportMessage("PhotonMapSampler", Logger::WARNING, "FresnelSampler", "FromNextPdf not supported");
    }

    // Component propagation
    if ( thisNode->m_rayType == REFLECTS ) {
        thisNode->m_usedComponents = BRDF_SPECULAR_COMPONENT;
    } else {
        thisNode->m_usedComponents = BTDF_SPECULAR_COMPONENT;
    }

    return true; // Node filled in
}

bool
PhotonMapSampler::gdSample(
    Camera *camera,
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneSceneQueryWorkspace,
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
    if ( m_photonMap == NULL ) {
        // We can just use standard bsdf sampling
        ok = BsdfSampler::sample(
            camera, sceneQuery, sceneSceneQueryWorkspace, sceneBackground, prevNode, thisNode, newNode, x1, x2, false, flags);
        thisNode->m_usedComponents = flags;
        return ok;
    }

    // -- Currently NEVER reached!

    // Choose G or D
    const PhongBidirScattDistFunc *bsdf = thisNode->m_useBsdf;
    bool dChosen;
    double pdfChoice;

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
    double glossy_exponent;

    if ( dChosen ) {
        // Equation [ARVO1995b](6) in CoordinateSystem::setFromZAxis builds the
        // local tangent basis used by the spherical importance map.
        coord.setFromZAxis(&thisNode->m_normal, VSDK::EPSILON);
        glossy_exponent = 1;
        flags = BRDF_DIFFUSE_COMPONENT;
    } else {
        flags = BRDF_GLOSSY_COMPONENT;

        Logger::reportMessage("PhotonMapSampler", Logger::ERROR, "PhotonMapSampler::gdSample", "Not done yet");
        return false;
    }

    double photonMapPdf = m_photonMap->sample(thisNode->m_hit.getPoint(), &x1, &x2, &coord, flags, glossy_exponent);

    // Do real sampling
    ok = BsdfSampler::sample(
        camera,
        sceneQuery,
        sceneSceneQueryWorkspace,
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

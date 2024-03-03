/* photonmapsampler.C: This is a hack to get fresnel factors
 * for perfect specular reflection and refraction
 */

#include <cmath>

#include "PHOTONMAP/photonmapsampler.h"
#include "material/bsdf.h"
#include "common/error.h"
#include "raycasting/common/raytools.h"

CPhotonMapSampler::CPhotonMapSampler() {
    m_photonMap = nullptr;
}

// Returns true a component was chosen, false if absorbed
bool
CPhotonMapSampler::ChooseComponent(BSDFFLAGS flags1, BSDFFLAGS flags2,
                                        BSDF *bsdf, RayHit *hit, bool doRR,
                                        double *x, float *pdf, bool *chose1) {
    COLOR col;
    float power1, power2, totalPower;

    // Choose between flags1 or flags2 scattering

    col = bsdfScatteredPower(bsdf, hit, &hit->geometricNormal, flags1);
    power1 = colorAverage(col);

    col = bsdfScatteredPower(bsdf, hit, &hit->geometricNormal, flags2);
    power2 = colorAverage(col);

    totalPower = power1 + power2;

    if ( totalPower < EPSILON ) {
        return false;
    } // No significant scattering...

    // Account for russian roulette

    if ( !doRR ) {
        power1 /= totalPower;
        power2 /= totalPower;
        totalPower = 1.0;
    }

    // use x for scattering choice

    if ( *x < power1 ) {
        *chose1 = true;
        *pdf = power1;
        *x = *x / power1;
    } else if ( *x < totalPower ) {
        *chose1 = false;
        *pdf = power2;
        *x = (*x - power1) / power2;
    } else {
        // Absorbed
        return false;
    }

    return true;
}

bool CPhotonMapSampler::Sample(CPathNode *prevNode, CPathNode *thisNode,
                               CPathNode *newNode, double x_1, double x_2,
                               bool doRR, BSDFFLAGS flags) {
    BSDF *bsdf = thisNode->m_useBsdf;
    bool sChosen;
    float pdfChoice;
    BSDFFLAGS sFlagMask;

    const XXDFFLAGS sFLAGS = BSDF_SPECULAR_COMPONENT;
    const XXDFFLAGS gdFLAGS = BRDF_GLOSSY_COMPONENT | BRDF_DIFFUSE_COMPONENT;

    // Choose between S or D|G scattering
    // Hack to get separate specular en fresnel ok...

    if ( flags == BRDF_SPECULAR_COMPONENT ) {
        sFlagMask = sFLAGS;  // Specular transmission can result in reflection
    } else {
        sFlagMask = flags;
    }
    if ( !ChooseComponent(
            (BSDFFLAGS)(sFLAGS & sFlagMask),
            (BSDFFLAGS)(gdFLAGS & flags),
            bsdf,
            &thisNode->m_hit,
            doRR,
            &x_2,
            &pdfChoice,
            &sChosen) ) {
        return false; // Absorbed
    }

    // Get a sampled direction

    bool ok;

    if ( sChosen ) {
        ok = FresnelSample(prevNode, thisNode, newNode, x_2, flags);
    } else {
        flags = (BSDFFLAGS)(gdFLAGS & flags);
        ok = GDSample(prevNode, thisNode, newNode, x_1, x_2,
                      false, flags);
    }

    if ( ok ) {
        // Adjust pdf with s vs gd choice
        newNode->m_pdfFromPrev *= pdfChoice;

        // Component propagation

        newNode->m_accUsedComponents = (BSDFFLAGS)(thisNode->m_accUsedComponents |
                                        thisNode->m_usedComponents);
    }

    return ok;
}


// The Fresnel sampler works as follows:
// 1) Index of refractions are taken
// 2) Reflectance and Transmittance values are taken. Normally one of the two
//    would be zero.
// 2b) Complex index of refraction, converted into geometric iof
// 3) Perfect reflected and refracted (if necessary) directions are computed
// 4) cosines and fresnel formulas are computed
// 5) reflection or refraction is chosen
// 6) fresnel reflection/refraction multiplied by appropriate scattering powers
// 7) node filled in.

// Utility functions

static RefractionIndex BsdfGeometricIOR(BSDF *bsdf) {
    RefractionIndex nc{};

    bsdfIndexOfRefraction(bsdf, &nc);

    // Convert to geometric IOR if necessary

    if ( nc.ni > EPSILON ) {
        nc.nr = complexToGeometricRefractionIndex(nc);
        nc.ni = 0.0; // ? Necessary ?
    }

    return nc;
}


static bool
ChooseFresnelDirection(
    CPathNode *thisNode,
    BSDFFLAGS flags,
    double x_2,
    Vector3D *dir,
    double *pdfDir,
    COLOR *scatteringColor,
    bool *doCosInverse)
{
    // Index of refractions are taken
    RefractionIndex nc_in{};
    RefractionIndex nc_out{}; // IOR

    nc_in = BsdfGeometricIOR(thisNode->m_inBsdf);
    nc_out = BsdfGeometricIOR(thisNode->m_outBsdf);

    // Reflectance and Transmittance values are taken. Normally one of the two
    // would be zero.

    BSDF *bsdf = thisNode->m_useBsdf;
    // Vector3D *point = &thisNode->m_hit.point;

    COLOR reflectance = bsdfSpecularReflectance(bsdf, &thisNode->m_hit,
                                                &thisNode->m_normal);
    COLOR transmittance = bsdfSpecularTransmittance(bsdf, &thisNode->m_hit,
                                                    &thisNode->m_normal);

    bool reflective = (colorAverage(reflectance) > EPSILON);
    bool transmittive = (colorAverage(transmittance) > EPSILON);

    if ( reflective && transmittive ) {
        logError("FresnelFactor",
                 "Cannot deal with simultaneous reflective & transittive materials");
        return false;
    }


    // Fresnel reflection factor is computed.

    float cosi, cost;
    float F;  // Fresnel reflection. (Refraction = 1 - T)
    int tir; // total internal reflection
    Vector3D reflectedDir, refractedDir;

    if ( reflective ) {
        if ( flags & BRDF_SPECULAR_COMPONENT ) {
            // Hack !?
            F = 1.0;
            reflectedDir = idealReflectedDirection(&thisNode->m_inDirT,
                                                   &thisNode->m_normal);
            cosi = vectorDotProduct(thisNode->m_normal, thisNode->m_inDirF);
            if ( cosi < 0 ) {
                logError("FresnelSample", "cosi < 0");
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

        // 4) cosines and fresnel formulas are computed

        cosi = vectorDotProduct(thisNode->m_normal, thisNode->m_inDirF);

        if ( cosi < 0 ) {
            logError("FresnelSample", "cosi < 0");
        }

        if ( !tir ) {
            cost = -vectorDotProduct(thisNode->m_normal, refractedDir);

            if ( cost < 0 ) {
                logError("FresnelSample", "cost < 0");
            }

            float rpar, rper;
            float nt = nc_out.nr;
            float ni = nc_in.nr;

            rpar = (nt * cosi - ni * cost) / (nt * cosi + ni * cost); // Parallel
            rper = (ni * cosi - nt * cost) / (ni * cosi + nt * cost); // Perpendicular

            F = 0.5f * (rpar * rpar + rper * rper);
        } else {
            F = 0;  // All in refracted dir, which == reflected dir
        }
    }

    float T = 1.0f - F;
    bool reflected;

    // choose reflection or refraction

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

    if ( sum < EPSILON ) {
        return false;
    }

    if ( x_2 < T / sum ) {
        reflected = false;
        *dir = refractedDir;
        x_2 = x_2 / (T / sum);
        *pdfDir = T / sum;
    } else {
        reflected = true;
        *dir = reflectedDir;
        x_2 = (x_2 - (T / sum)) / (F / sum);
        *pdfDir = F / sum;
    }

    // Compute bsdf evaluation here and determine if we
    // still need to divide by the cosine to get 'real'
    // specular transmission

    if ( reflected ) {
        if ( reflective ) {
            colorScale(F, reflectance, *scatteringColor);
            *doCosInverse = false;
        } else {
            colorScale(F, transmittance, *scatteringColor);
            *doCosInverse = true;
        }
    } else {
        colorScale(T, transmittance, *scatteringColor);
        *doCosInverse = true;
    }

    return true;
}

bool
CPhotonMapSampler::FresnelSample(
    CPathNode *prevNode,
    CPathNode *thisNode,
    CPathNode *newNode,
    double x_2,
    BSDFFLAGS flags)
{
    Vector3D dir;
    double pdfDir;
    bool doCosInverse;
    COLOR scatteringColor;

    if ( !ChooseFresnelDirection(thisNode, flags, x_2, &dir, &pdfDir,
                                 &scatteringColor, &doCosInverse)) {
        return false;
    }

    // Reflection Type, changes thisNode->m_rayType and newNode->m_inBsdf
    DetermineRayType(thisNode, newNode, &dir);

    // Transfer
    if ( !SampleTransfer(thisNode, newNode, &dir, pdfDir)) {
        thisNode->m_rayType = Stops;
        return false;
    }

    // Fill in bsdf evaluation. This is just reflectance or transmittance
    // given by ChooseFresnelDirection, but may be divided by a cosine
    // -- No bsdf components yet here !!

    if ( doCosInverse ) {
        float cosb = fabs(vectorDotProduct(newNode->m_hit.normal,
                                           newNode->m_inDirT));
        colorScaleInverse(cosb, scatteringColor, thisNode->m_bsdfEval);
    } else {
        thisNode->m_bsdfEval = scatteringColor;
    }


    // Fill in probability for previous node

    if ( m_computeFromNextPdf && prevNode ) {
        logWarning("FresnelSampler", "FromNextPdf not supported");
    }

    // Component propagation
    if ( thisNode->m_rayType == Reflects ) {
        thisNode->m_usedComponents = BRDF_SPECULAR_COMPONENT;
    } else {
        thisNode->m_usedComponents = BTDF_SPECULAR_COMPONENT;
    }

    return true; // Node filled in
}


bool CPhotonMapSampler::GDSample(CPathNode *prevNode, CPathNode *thisNode,
                                 CPathNode *newNode, double x_1, double x_2,
                                 bool doRR, BSDFFLAGS flags) {
    bool ok;

    // Sample G|D and use m_photonMap for importance sampling if possible.

    if ( m_photonMap == nullptr ) {
        // We can just use standard bsdf sampling
        ok = CBsdfSampler::Sample(prevNode, thisNode, newNode, x_1, x_2,
                                  doRR, flags);
        thisNode->m_usedComponents = flags;
        return ok;
    }

    // -- Currently NEVER reached!

    // Choose G or D

    BSDF *bsdf = thisNode->m_useBsdf;
    bool dChosen;
    float pdfChoice;

    // Choose between D or G scattering

    if ( !ChooseComponent(BRDF_DIFFUSE_COMPONENT & flags,
                          BRDF_GLOSSY_COMPONENT & flags,
                          bsdf, &thisNode->m_hit,
                          doRR, &x_1, &pdfChoice, &dChosen)) {
        return false;
    }

    // Importance sampling using photon map x_1 & x_2 get transformed

    COORDSYS coord;
    float glossy_exponent;

    if ( dChosen ) {
        vectorCoordSys(&thisNode->m_normal, &coord);
        glossy_exponent = 1;
        flags = BRDF_DIFFUSE_COMPONENT;
    } else {
        flags = BRDF_GLOSSY_COMPONENT;

        logError("CPhotonMapSampler::GDSample", "Shit nog nie klaar");
        return false;
    }

    double pmapPdf = m_photonMap->Sample(thisNode->m_hit.point, &x_1, &x_2, &coord, flags,
                                         glossy_exponent);

    // Do real sampling

    ok = CBsdfSampler::Sample(prevNode, thisNode, newNode, x_1, x_2,
                              false, flags);

    // Adjust pdf's

    if ( ok ) {
        newNode->m_pdfFromPrev *= pdfChoice * pmapPdf;
        thisNode->m_usedComponents = flags;
    }

    return ok;
}




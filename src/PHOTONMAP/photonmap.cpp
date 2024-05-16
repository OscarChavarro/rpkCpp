#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "java/lang/Math.h"
#include "common/error.h"
#include "common/Statistics.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "PHOTONMAP/photonmap.h"

bool
zeroAlbedo(const PhongBidirectionalScatteringDistributionFunction *bsdf, RayHit *hit, char flags) {
    ColorRgb color;
    if ( bsdf == nullptr ) {
        color.clear();
    } else {
        color = bsdf->splitBsdfScatteredPower(hit, flags);
    }
    return (color.average() < EPSILON);
}

static float
getFalseMonochrome(float val) {
    float tmp;

    float max = GLOBAL_photonMap_state.falseColMax;

    if ( GLOBAL_photonMap_state.falseColLog ) {
        max = (float)java::Math::log(1.0 + max);
        val = (float)java::Math::log(1.0 + val);
    }

    tmp = java::Math::min(val, max);
    tmp = (tmp / max);

    return tmp;
}

ColorRgb
getFalseColor(float val) {
    ColorRgb col;
    float max;
    float tmp;
    float r = 0;
    float g = 0;
    float b = 0;

    if ( GLOBAL_photonMap_state.falseColMono ) {
        tmp = getFalseMonochrome(val);
        col.set(tmp, tmp, tmp);
        return col;
    }

    max = GLOBAL_photonMap_state.falseColMax;

    if ( GLOBAL_photonMap_state.falseColLog ) {
        max = (float)java::Math::log(1.0 + max);
        val = (float)java::Math::log(1.0 + val);
    }

    tmp = java::Math::min(val, max);

    // Do some log scale ?

    tmp = 3.0f * (tmp / max);

    if ( tmp <= 1.0 ) {
        b = tmp;
    } else if ( tmp < 2.0 ) {
        g = tmp - 1.0f;
        b = 1.0f - g;
    } else {
        r = tmp - 2.0f;
        g = 1.0f - r;
    }

    col.set(r, g, b);
    return col;
}

CPhotonMap::CPhotonMap(int *estimate_nrp, bool doPrecomputeIrradiance):
    m_sample_nrp(), m_nrpCosinePos()
{
    m_balanced = true;
    m_doBalancing = false;

    m_estimate_nrp = estimate_nrp;

    m_precomputeIrradiance = doPrecomputeIrradiance;
    m_irradianceComputed = false;

    if ( doPrecomputeIrradiance ) {
        m_kdtree = new CPhotonkdtree(sizeof(CIrrPhoton), true);
    } else {
        m_kdtree = new CPhotonkdtree(sizeof(CPhoton), true);
    }

    m_totalPaths = 0;
    m_nrPhotons = 0;
    m_totalPhotons = 0;

    m_grid = new CSampleGrid2D(2, 4);
    m_sampleLastPos.set(HUGE_FLOAT_VALUE, HUGE_FLOAT_VALUE, HUGE_FLOAT_VALUE);

    m_photons = new CPhoton *[MAXIMUM_RECON_PHOTONS];
    m_distances = new float[MAXIMUM_RECON_PHOTONS];
    m_cosines = new float[MAXIMUM_RECON_PHOTONS];

    m_nrpFound = 0;  // No valid photons in array
    m_cosinesOk = true;
}

CPhotonMap::~CPhotonMap() {
    delete m_kdtree;
    delete[] m_photons;
    delete[] m_distances;
    delete[] m_cosines;
}

void
CPhotonMap::computeCosines(const Vector3D normal) {
    Vector3D dir;

    if ( !m_cosinesOk ) {
        m_nrpCosinePos = 0;

        for ( int i = 0; i < m_nrpFound; i++ ) {
            dir = m_photons[i]->dir();
            m_cosines[i] = dir.dotProduct(normal);
            if ( m_cosines[i] > 0 ) {
                m_nrpCosinePos++;
            }
        }

        m_cosinesOk = true;
    }
}

// Adding photons
void
CPhotonMap::doAddPhoton(
    CPhoton &photon,
    Vector3D normal,
    short flags)
{
    if ( m_precomputeIrradiance ) {
        CIrrPhoton irrPhoton;
        irrPhoton.copy(photon);
        irrPhoton.setNormal(normal);
        m_kdtree->addPoint(&irrPhoton, flags);
    } else {
        m_kdtree->addPoint(&photon, flags);
    }
}

/**
Adding photons, returns if photon was added
*/
bool
CPhotonMap::addPhoton(CPhoton &photon, Vector3D normal, short flags) {
    drand48(); // Just to keep in sync with density controlled storage

    doAddPhoton(photon, normal, flags);
    m_nrPhotons++;
    m_totalPhotons++;
    m_balanced = false;
    m_irradianceComputed = false;

    return true;
}

double
ComputeAcceptProb(float currentD, float requiredD) {
    // Step function
    if ( GLOBAL_photonMap_state.acceptPdfType == STEP ) {
        if ( currentD > requiredD ) {
            return 0.0;
        } else {
            return 1.0;
        }
    } else if ( GLOBAL_photonMap_state.acceptPdfType == TRANS_COSINE ) {
        // Translated cosine
        double ratio = java::Math::min(1.0, currentD / requiredD); // in [0,1]

        return (0.5 * (1.0 + java::Math::cos(ratio * M_PI)));
    } else {
        logError("ComputeAcceptProb", "Unknown accept pdf type");
        return 0.0;
    }
}

void
CPhotonMap::redistribute(const CPhoton &photon) const {
    // redistribute this photon over the nearest neighbours
    // m_distances, m_photons and m_cosines should be filled correctly!
    // only photons are used for which direction * normal > 0

    // -- Check the flags
    // -- normal weighted average?

    ColorRgb deltaPower;
    ColorRgb pow;
    float factor = 1.0f / (float)m_nrpCosinePos;

    pow = photon.power();
    deltaPower.scaledCopy(factor, pow);

    for ( int i = 0; i < m_nrpFound; i++ ) {
        if ( m_cosines[i] > 0.0 ) {
            m_photons[i]->addPower(deltaPower);
        }
    }
}

bool
CPhotonMap::DC_AddPhoton(
    CPhoton &photon,
    RayHit &hit,
    float requiredD,
    short flags)
{
    // Get current density
    // Vector3D pos = photon.Pos();
    bool stored;

    float currentD = getCurrentDensity(hit, GLOBAL_photonMap_state.distribPhotons);
    // m_photons and m_distances is valid now !!

    // Compute acceptance probability

    double acceptProb = ComputeAcceptProb(currentD, requiredD);

    // printf("A prob %g, CD %g RD %g\n", acceptProb, currentD, requiredD);

    // Roulette
    if ( drand48() < acceptProb ) {
        // Store
        doAddPhoton(photon, hit.getNormal(), flags);
        m_nrPhotons++;
        m_balanced = false;
        m_irradianceComputed = false;
        stored = true;
    } else {
        // redistribute power over neighbours or ignore
        stored = false;
        redistribute(photon);
    }

    m_totalPhotons++; // All photons including non stored photons

    return stored;
}

/**
Get a maximum radius^2 to be used when locating photons
*/
double
CPhotonMap::GetMaxR2() {
    /* A maximum radius^2 is chosen as follows:
     * The radiance of the reconstruction must be larger
     * than a fraction of the radiance contribution when
     * taking into account all the stored photons (N_all) (some kind
     * of ambient radiance). R_all is the radius including all photons.
     * R_all^2 / N_all is approximated by GLOBAL_statistics_totalArea / M_PI * m_totalPaths
     * (This is an over estimation, which is ok for a maxr estimate)
     * BRDF eval are approximated by 1.
     */
    const double radFraction = 0.03;

    double maxr2 = ((double)*m_estimate_nrp * GLOBAL_statistics.totalArea /
                    (M_PI * (double)m_totalPaths * radFraction));

    return maxr2;
}

// Precompute Irradiance
void
CPhotonMap::photonPrecomputeIrradiance(Camera *camera, CIrrPhoton *photon) {
    ColorRgb irradiance;
    ColorRgb power;
    irradiance.clear();

    // Locate the nearest photons using a max radius limit
    Vector3D pos = photon->pos();
    m_nrpFound = doQuery(&pos);

    if ( m_nrpFound > 3 ) {
        // Construct irradiance estimate
        float maxDistance = m_distances[0];

        for ( int i = 0; i < m_nrpFound; i++ ) {
            if ( photon->Normal().dotProduct(m_photons[i]->dir()) > 0 ) {
                power = m_photons[i]->power();
                irradiance.add(irradiance, power);
            }
        }

        // Now we have incoming radiance integrated over area estimate,
        // so we convert it to irradiance, maxDistance is already squared
        // An extra factor PI is added, that accounts for Albedo -> diffuse brdf...
        float factor = (1.0f / ((float)M_PI * (float)M_PI * maxDistance * (float)m_totalPaths));
        irradiance.scale(factor);
    }

    photon->SetIrradiance(irradiance);
}

static void
PrecomputeIrradianceCallback(Camera *camera, CPhotonMap *map, CIrrPhoton *photon) {
    map->photonPrecomputeIrradiance(camera, photon);
}

void
CPhotonMap::precomputeIrradiance() {
    fprintf(stderr, "CPhotonMap::precomputeIrradiance\n");
    if ( m_precomputeIrradiance && !m_irradianceComputed ) {
        m_kdtree->iterateNodes((void (*)(void *, void *)) PrecomputeIrradianceCallback, this);
        m_irradianceComputed = true;
    }
}

bool
CPhotonMap::irradianceReconstruct(
    RayHit *hit,
    const Vector3D &outDir /*outDir*/,
    const ColorRgb &diffuseAlbedo,
    ColorRgb *result)
{
    if ( !m_irradianceComputed ) {
        precomputeIrradiance();
    }

    Vector3D normal = hit->getNormal();
    Vector3D position = hit->getPoint();
    const CIrrPhoton *photon = DoIrradianceQuery(&position, &normal);
    hit->setNormal(&normal);

    if ( photon ) {
        result->scalarProduct(photon->m_irradiance, diffuseAlbedo);
        return true;
    } else {
        // No appropriate photon found
        return false;
    }
}

ColorRgb
CPhotonMap::reconstruct(
    RayHit *hit,
    Vector3D &outDir,
    PhongBidirectionalScatteringDistributionFunction *bsdf,
    PhongBidirectionalScatteringDistributionFunction *inBsdf,
    PhongBidirectionalScatteringDistributionFunction *outBsdf)
{
    // Find the nearest photons
    float maxDistance;
    ColorRgb result;
    ColorRgb eval;
    ColorRgb power;
    ColorRgb col;
    float factor;

    result.clear();

    ColorRgb diffuseAlbedo;
    ColorRgb glossyAlbedo;

    diffuseAlbedo.clear();
    glossyAlbedo.clear();

    if ( bsdf != nullptr ) {
        diffuseAlbedo = bsdf->splitBsdfScatteredPower(hit, BRDF_DIFFUSE_COMPONENT);
        // -- TODO Irradiance pre-computation for diffuse transmission
        glossyAlbedo = bsdf->splitBsdfScatteredPower(hit, BTDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT);
    }

    checkNBalance();

    if ( glossyAlbedo.average() < EPSILON ) {
        if ( diffuseAlbedo.average() < EPSILON ) {
            return result; // No reflectance
        } else {
            if ( m_precomputeIrradiance ) {
                if ( irradianceReconstruct(hit, outDir, diffuseAlbedo, &result) ) {
                    return result;
                } else {
                    // No appropriate irradiance photon -> do normal reconstruction
                }
            }
        }
    }

    // Normal reconstruct...

    // Locate nearest photons using a max radius limit
    Vector3D position = hit->getPoint();
    m_nrpFound = doQuery(&position);

    if ( m_nrpFound < 3 ) {
        return result;
    }

    // Construct radiance estimate
    maxDistance = m_distances[0];

    for ( int i = 0; i < m_nrpFound; i++ ) {
        Vector3D dir = m_photons[i]->dir();

        if ( bsdf == nullptr ) {
            eval.clear();
        } else {
            eval = bsdf->evaluate(
                hit, inBsdf, outBsdf, &outDir, &dir, BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT);
        }
        power = m_photons[i]->power();

        col.scalarProduct(eval, power);
        result.add(result, col);
    }

    // Now we have a radiance integrated over area estimate,
    // so we convert it to radiance, maxDistance is already squared

    factor = 1.0f / ((float)M_PI * maxDistance * (float)m_totalPaths);

    result.scale(factor);

    return result;
}

float
CPhotonMap::getCurrentDensity(RayHit &hit, int nrPhotons) {
    // Find the nearest photons
    if ( nrPhotons == 0 ) {
        nrPhotons = *m_estimate_nrp;
    }

    float maxDistance;

    if ( nrPhotons == 0 ) {
        return 0.0;
    }

    Vector3D position = hit.getPoint();
    m_nrpFound = doQuery(&position, nrPhotons, (float) GetMaxR2());

    if ( m_nrpFound < 3 ) {
        return 0.0;
    }

    // Construct density estimate
    maxDistance = m_distances[0]; // Only valid since max heap is used in kdtree

    computeCosines(hit.getGeometricNormal()); // Shading normal?

    if ( m_nrpCosinePos <= 3 ) {
        return 0.0;
    }

    return (float)(m_nrpCosinePos / (M_PI * maxDistance));
}

/**
Return a color coded density of the photon map
*/
ColorRgb
CPhotonMap::getDensityColor(RayHit &hit) {
    float density;
    ColorRgb result;

    density = getCurrentDensity(hit, 0);

    result = getFalseColor(density);

    return result;
}

double
CPhotonMap::sample(
    Vector3D position,
    double *r,
    double *s,
    const CoordinateSystem *coord,
    char flag,
    float n)
{
    ColorRgb color;

    // -- Epsilon in as a function of scene/camera measure ??
    if ( !m_sampleLastPos.equals(position, 0.0001f) ) {
        // Need a new grid

        m_grid->init();

        // Find the nearest photons
        m_nrpFound = doQuery(&position, m_sample_nrp, KD_MAX_RADIUS, NO_IMPSAMP_PHOTON);

        double pr;
        double ps;

        for ( int i = 0; i < m_nrpFound; i++ ) {
            m_photons[i]->findRS(&pr, &ps, coord, flag, n);

            color = m_photons[i]->power();

            m_grid->add(pr, ps, color.average() / (float)m_nrPhotons);
        }

        m_grid->EnsureNonZeroEntries();
        m_sampleLastPos = position; // Caching
    }

    // Sample
    double probabilityDensityFunction;
    m_grid->sample(r, s, &probabilityDensityFunction);

    return probabilityDensityFunction;
}

#endif

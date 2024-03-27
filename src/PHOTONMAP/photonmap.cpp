#include "common/error.h"
#include "material/statistics.h"
#include "PHOTONMAP/photonmap.h"

bool
ZeroAlbedo(BSDF *bsdf, RayHit *hit, BSDF_FLAGS flags) {
    COLOR col = bsdfScatteredPower(bsdf, hit, &hit->geometricNormal, flags);
    return (colorAverage(col) < EPSILON);
}

static float
GetFalseMonochrome(float val) {
    float tmp;

    float max = GLOBAL_photonMap_state.falseColMax;

    if ( GLOBAL_photonMap_state.falseColLog ) {
        max = (float)std::log(1.0 + max);
        val = (float)std::log(1.0 + val);
    }

    tmp = floatMin(val, max);
    tmp = (tmp / max);

    return tmp;
}

COLOR
GetFalseColor(float val) {
    RGB rgb{};
    COLOR col;
    float max;
    float tmp;
    float r = 0;
    float g = 0;
    float b = 0;

    if ( GLOBAL_photonMap_state.falseColMono ) {
        tmp = GetFalseMonochrome(val);
        setRGB(rgb, tmp, tmp, tmp);
        convertRGBToColor(rgb, &col);
        return col;
    }

    max = GLOBAL_photonMap_state.falseColMax;

    if ( GLOBAL_photonMap_state.falseColLog ) {
        max = (float)std::log(1.0 + max);
        val = (float)std::log(1.0 + val);
    }

    tmp = floatMin(val, max);

    // Do some log scale ?

    tmp = 3.0f * (tmp / max);

    if ( tmp <= 1.0 ) {
        b = tmp;
    } else if ( tmp < 2.0 ) {
        g = (float)tmp - 1.0f;
        b = 1.0f - (float)g;
    } else {
        r = (float)tmp - 2.0f;
        g = 1.0f - (float)r;
    }

    setRGB(rgb, r, g, b);
    convertRGBToColor(rgb, &col);

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
    m_sampleLastPos.set(HUGE, HUGE, HUGE);

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
CPhotonMap::Init() {
}

void
CPhotonMap::ComputeCosines(Vector3D &normal) {
    Vector3D dir;

    if ( !m_cosinesOk ) {
        m_nrpCosinePos = 0;

        for ( int i = 0; i < m_nrpFound; i++ ) {
            dir = m_photons[i]->Dir();
            m_cosines[i] = vectorDotProduct(dir, normal);
            if ( m_cosines[i] > 0 ) {
                m_nrpCosinePos++;
            }
        }

        m_cosinesOk = true;
    }
}

// Adding photons
void CPhotonMap::DoAddPhoton(CPhoton &photon, Vector3D &normal,
                             short flags) {
    if ( m_precomputeIrradiance ) {
        CIrrPhoton irrPhoton;
        irrPhoton.Copy(photon);
        irrPhoton.SetNormal(normal);
        m_kdtree->addPoint(&irrPhoton, flags);
    } else {
        m_kdtree->addPoint(&photon, flags);
    }
}

bool
CPhotonMap::AddPhoton(CPhoton &photon, Vector3D &normal, short flags) {
    drand48(); // Just to keep in sync with density controlled storage

    DoAddPhoton(photon, normal, flags);
    //m_kdtree->addPoint(&photon, flags);
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
        double ratio = floatMin(1.0, currentD / requiredD); // in [0,1]

        return (0.5 * (1.0 + std::cos(ratio * M_PI)));
    } else {
        logError("ComputeAcceptProb", "Unknown accept pdf type");
        return 0.0;
    }
}

void
CPhotonMap::Redistribute(CPhoton &photon) {
    // Redistribute this photon over the nearest neighbours
    // m_distances, m_photons and m_cosines should be filled correctly!
    // only photons are used for which direction * normal > 0

    // -- Check the flags
    // -- normal weighted average?

    COLOR deltaPower;
    COLOR pow;
    float factor = 1.0f / (float)m_nrpCosinePos;

    pow = photon.Power();
    colorScale(factor, pow, deltaPower);

    for ( int i = 0; i < m_nrpFound; i++ ) {
        if ( m_cosines[i] > 0.0 ) {
            m_photons[i]->AddPower(deltaPower);
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

    float currentD = GetCurrentDensity(hit, GLOBAL_photonMap_state.distribPhotons);
    // m_photons and m_distances is valid now !!

    // Compute acceptance probability

    double acceptProb = ComputeAcceptProb(currentD, requiredD);

    // printf("A prob %g, CD %g RD %g\n", acceptProb, currentD, requiredD);

    // Roulette
    if ( drand48() < acceptProb ) {
        // Store
        DoAddPhoton(photon, hit.normal, flags);
        m_nrPhotons++;
        m_balanced = false;
        m_irradianceComputed = false;
        stored = true;
    } else {
        // Redistribute power over neighbours or ignore
        stored = false;
        Redistribute(photon);
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
CPhotonMap::PhotonPrecomputeIrradiance(CIrrPhoton *photon) {
    COLOR irradiance, power;
    colorClear(irradiance);

    // locate nearest photons using a max radius limit

    Vector3D pos = photon->Pos();
    m_nrpFound = DoQuery(&pos);

    if ( m_nrpFound > 3 ) {
        // Construct irradiance estimate
        float maxDistance = m_distances[0];

        for ( int i = 0; i < m_nrpFound; i++ ) {
            if ((photon->Normal() & m_photons[i]->Dir()) > 0 ) {
                power = m_photons[i]->Power();
                colorAdd(irradiance, power, irradiance);
            }
        }

        // Now we have incoming radiance integrated over area estimate,
        // so we convert it to irradiance, maxDistance is already squared
        // An extra factor PI is added, that accounts for Albedo -> diffuse brdf...
        float factor = (float)(1.0f / ((float)M_PI * (float)M_PI * maxDistance * (float)m_totalPaths));
        colorScale(factor, irradiance, irradiance);
    }

    photon->SetIrradiance(irradiance);
}

static void
PrecomputeIrradianceCallback(CPhotonMap *map, CIrrPhoton *photon) {
    map->PhotonPrecomputeIrradiance(photon);
}

void
CPhotonMap::PrecomputeIrradiance() {
    fprintf(stderr, "CPhotonMap::PrecomputeIrradiance\n");
    if ( m_precomputeIrradiance && !m_irradianceComputed ) {
        m_kdtree->iterateNodes((void (*)(void *, void *)) PrecomputeIrradianceCallback,
                               this);
        m_irradianceComputed = true;
    }
}

bool
CPhotonMap::IrradianceReconstruct(
    RayHit *hit,
    Vector3D &outDir,
    COLOR &diffuseAlbedo,
    COLOR *result)
{
    if ( !m_irradianceComputed ) {
        PrecomputeIrradiance();
    }

    CIrrPhoton *photon = DoIrradianceQuery(&hit->point, &hit->normal);

    if ( photon ) {
        //float factor = 1.0 / (M_PI * m_totalPaths);
        //colorProductScaled(photon->m_irradiance, factor, diffuseAlbedo, *result);

        colorProduct(photon->m_irradiance, diffuseAlbedo, *result);

        // COLOR eval = BsdfEval(bsdf, hit, inBsdf, outBsdf, &outDir,
        // &hit->normal, BRDF_DIFFUSE_COMPONENT);
        // colorProduct(photon->m_irradiance, eval, *result);
        return true;
    } else {
        // No appropriate photon found
        return false;
    }
}

COLOR
CPhotonMap::Reconstruct(RayHit *hit, Vector3D &outDir,
                              BSDF *bsdf, BSDF *inBsdf, BSDF *outBsdf) {
    // Find the nearest photons
    float maxDistance;
    COLOR result, eval, power, col;
    float factor;

    colorClear(result);

    COLOR diffuseAlbedo, glossyAlbedo;

    diffuseAlbedo = bsdfScatteredPower(bsdf, hit, &hit->geometricNormal, BRDF_DIFFUSE_COMPONENT);
    // -- TODO Irradiance pre-computation for diffuse transmission
    glossyAlbedo = bsdfScatteredPower(bsdf, hit, &hit->geometricNormal, BTDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT);

    CheckNBalance();

    if ( colorAverage(glossyAlbedo) < EPSILON ) {
        if ( colorAverage(diffuseAlbedo) < EPSILON ) {
            return result; // No reflectance
        } else {
            if ( m_precomputeIrradiance ) {
                // if(IrradianceReconstruct(hit, outDir, bsdf, inBsdf, outBsdf, &result))
                if ( IrradianceReconstruct(hit, outDir, diffuseAlbedo, &result)) {
                    return result;
                } else // no appropriate irradiance photon -> do normal reconstruction
                {
                    // fprintf(stderr, "No irradiance photon found\n");
                }
            }
        }
    }

    // Normal reconstruct...

    // locate nearest photons using a max radius limit
    m_nrpFound = DoQuery(&hit->point);

    if ( m_nrpFound < 3 ) {
        return result;
    }

    // Construct radiance estimate

    maxDistance = m_distances[0];

    for ( int i = 0; i < m_nrpFound; i++ ) {
        Vector3D dir = m_photons[i]->Dir();
        eval = bsdfEval(bsdf, hit, inBsdf, outBsdf, &outDir,
                        &dir,
                        BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT);
        power = m_photons[i]->Power();

        colorProduct(eval, power, col);
        colorAdd(result, col, result);
    }

    // Now we have a radiance integrated over area estimate,
    // so we convert it to radiance, maxDistance is already squared

    factor = 1.0f / ((float)M_PI * maxDistance * (float)m_totalPaths);

    colorScale(factor, result, result);

    return result;
}

float
CPhotonMap::GetCurrentDensity(RayHit &hit, int nrPhotons) {
    // Find the nearest photons
    if ( nrPhotons == 0 ) {
        nrPhotons = *m_estimate_nrp;
    }

    float maxDistance;

    if ( nrPhotons == 0 ) {
        return 0.0;
    }

    m_nrpFound = DoQuery(&hit.point, nrPhotons, (float)GetMaxR2());

    if ( m_nrpFound < 3 ) {
        return 0.0;
    }

    // Construct density estimate
    maxDistance = m_distances[0]; // Only valid since max heap is used in kdtree

    ComputeCosines(hit.geometricNormal); // -- shading normal ?

    if ( m_nrpCosinePos <= 3 ) {
        return 0.0;
    }

    return (float)(m_nrpCosinePos / (M_PI * maxDistance));
}

/**
Return a color coded density of the photon map
*/
COLOR
CPhotonMap::GetDensityColor(RayHit &hit) {
    float density;
    COLOR result;

    density = GetCurrentDensity(hit, 0);

    result = GetFalseColor(density);

    return result;
}

double
CPhotonMap::Sample(
        Vector3D &pos,
        double *r,
        double *s,
        CoordSys *coord,
        BSDF_FLAGS flag,
        float n)
{
    COLOR col;

    // -- Epsilon in as a function of scene/camera measure ??
    if ( !vectorEqual(m_sampleLastPos, pos, 0.0001)) {
        // Need a new grid

        m_grid->Init();

        // Find the nearest photons

        m_nrpFound = DoQuery(&pos, m_sample_nrp, KD_MAX_RADIUS, NO_IMPSAMP_PHOTON);

        //printf("m_nrpFound %i\n", m_nrpFound);

        double pr, ps;

        for ( int i = 0; i < m_nrpFound; i++ ) {
            m_photons[i]->FindRS(&pr, &ps, coord, flag, n);

            col = m_photons[i]->Power();

            m_grid->Add(pr, ps, colorAverage(col) / (float)m_nrPhotons);
        }

        m_grid->EnsureNonZeroEntries();
        m_sampleLastPos = pos; // Caching
    }

    // Sample
    double probabilityDensityFunction;
    m_grid->Sample(r, s, &probabilityDensityFunction);

    return probabilityDensityFunction;
}

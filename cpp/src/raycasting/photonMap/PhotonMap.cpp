#include <cstdlib>

#include "java/util/Formatter.h"
#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "java/lang/System.h"
#include "common/Error.h"
#include "common/statistics/Statistics.h"
#include "raycasting/photonMap/Photon.h"
#include "raycasting/photonMap/PhotonMap.h"
bool
PhotonMap::zeroAlbedo(const PhongBidirectionalScatteringDistributionFunction *bsdf, RayHit *hit, char flags) {
    ColorRgb color;
    if ( bsdf == nullptr ) {
        color.clear();
    } else {
        color = bsdf->splitBsdfScatteredPower(hit, flags);
    }
    return (color.average() < Numeric::EPSILON);
}

float
PhotonMap::getFalseMonochrome(float val, const PhotonMapState &photonMapState) {
    float max = photonMapState.falseColMax;

    if ( photonMapState.falseColLog ) {
        max = static_cast<float>(java::Math::log(1.0 + max));
        val = static_cast<float>(java::Math::log(1.0 + val));
    }

    float tmp = java::Math::min(val, max);
    tmp = (tmp / max);

    return tmp;
}

ColorRgb
PhotonMap::getFalseColor(float val, const PhotonMapState &photonMapState) {
    ColorRgb col;
    float tmp;
    float r = 0;
    float g = 0;
    float b = 0;

    if ( photonMapState.falseColMono ) {
        tmp = PhotonMap::getFalseMonochrome(val, photonMapState);
        col.set(tmp, tmp, tmp);
        return col;
    }

    float max = photonMapState.falseColMax;

    if ( photonMapState.falseColLog ) {
        max = static_cast<float>(java::Math::log(1.0 + max));
        val = static_cast<float>(java::Math::log(1.0 + val));
    }

    tmp = java::Math::min(val, max);

    // Does some log scale ?

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

PhotonMap::PhotonMap(
    PhotonMapState &inPhotonMapState,
    int *estimate_nrp,
    bool doPrecomputeIrradiance):
    photonMapState(inPhotonMapState),
    m_sample_nrp(), m_nrpCosinePos()
{
    m_balanced = true;
    m_doBalancing = false;

    m_estimate_nrp = estimate_nrp;

    m_precomputeIrradiance = doPrecomputeIrradiance;
    m_irradianceComputed = false;

    if ( doPrecomputeIrradiance ) {
        m_kdtree = new PhotonKDTree(sizeof(IrrPhoton), true);
    } else {
        m_kdtree = new PhotonKDTree(sizeof(Photon), true);
    }

    m_totalPaths = 0;
    m_nrPhotons = 0;
    m_totalPhotons = 0;

    m_grid = new SampleGrid2D(2, 4);
    m_sampleLastPos.set(Numeric::HUGE_FLOAT_VALUE, Numeric::HUGE_FLOAT_VALUE, Numeric::HUGE_FLOAT_VALUE);

    m_photons = new Photon *[PhotonMapState::MAXIMUM_RECON_PHOTONS];
    m_distances = new float[PhotonMapState::MAXIMUM_RECON_PHOTONS];
    m_cosines = new float[PhotonMapState::MAXIMUM_RECON_PHOTONS];

    m_nrpFound = 0;  // No valid photons in array
    m_cosinesOk = true;
}

PhotonMap::~PhotonMap() {
    delete m_kdtree;
    delete[] m_photons;
    delete[] m_distances;
    delete[] m_cosines;
}

void
PhotonMap::printStats(java::PrintStream *stream) const {
    if ( stream == nullptr ) {
        return;
    }
    stream->printf("%i stored photons\n", m_nrPhotons);
}

void
PhotonMap::getStats(char *p, int n) const {
    java::Formatter::format(
        p, n, "%i stored photons, %i total, %li paths\n", m_nrPhotons, m_totalPhotons, m_totalPaths);
}

void
PhotonMap::computeCosines(const Vector3D normal) {
    if ( !m_cosinesOk ) {
        m_nrpCosinePos = 0;

        for ( int i = 0; i < m_nrpFound; i++ ) {
            Vector3D dir = m_photons[i]->dir();
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
PhotonMap::doAddPhoton(
    Photon &photon,
    Vector3D normal,
    short flags)
{
    if ( m_precomputeIrradiance ) {
        IrrPhoton irrPhoton;
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
PhotonMap::addPhoton(Photon &photon, Vector3D normal, short flags) {
    drand48(); // Just to keep in sync with density controlled storage

    doAddPhoton(photon, normal, flags);
    m_nrPhotons++;
    m_totalPhotons++;
    m_balanced = false;
    m_irradianceComputed = false;

    return true;
}

double
PhotonMap::computeAcceptProb(
    float currentD,
    float requiredD,
    const PhotonMapState &photonMapState) {
    // Step function
    if ( photonMapState.acceptPdfType == PhotonMapDCAcceptPDFType::STEP ) {
        if ( currentD > requiredD ) {
            return 0.0;
        } else {
            return 1.0;
        }
    } else if ( photonMapState.acceptPdfType == PhotonMapDCAcceptPDFType::TRANS_COSINE ) {
        // Translated cosine
        double ratio = java::Math::min(1.0, currentD / requiredD); // in [0,1]

        return (0.5 * (1.0 + java::Math::cos(ratio * M_PI)));
    } else {
        Error::error("PhotonMap::computeAcceptProb", "Unknown accept pdf type");
        return 0.0;
    }
}

void
PhotonMap::redistribute(const Photon &photon) const {
    // redistribute this photon over the nearest neighbours
    // m_distances, m_photons and m_cosines should be filled correctly!
    // only photons are used for which direction * normal > 0

    // -- Check the flags
    // -- normal weighted average?

    ColorRgb deltaPower;
    float factor = 1.0f / static_cast<float>(m_nrpCosinePos);

    ColorRgb pow = photon.power();
    deltaPower.scaledCopy(factor, pow);

    for ( int i = 0; i < m_nrpFound; i++ ) {
        if ( m_cosines[i] > 0.0 ) {
            m_photons[i]->addPower(deltaPower);
        }
    }
}

bool
PhotonMap::DC_AddPhoton(
    Photon &photon,
    RayHit &hit,
    float requiredD,
    short flags)
{
    // Get current density
    // Vector3D pos = photon.Pos();
    bool stored;

    float currentD = getCurrentDensity(hit, photonMapState.distribPhotons);
    // m_photons and m_distances is valid now !!

    // Compute acceptance probability

    double acceptProb = PhotonMap::computeAcceptProb(currentD, requiredD, photonMapState);

    // Debug trace for acceptance probability and density values.

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
PhotonMap::GetMaxR2() {
    /* A maximum radius^2 is chosen as follows:
     * The radiance of the reconstruction must be larger
     * than a fraction of the radiance contribution when
     * taking into account all the stored photons (N_all) (some kind
     * of ambient radiance). R_all is the radius including all photons.
     * R_all^2 / N_all is approximated by
     * Statistics::instance().radiance.totalArea / M_PI * m_totalPaths
     * (This is an over estimation, which is ok for a maxr estimate)
     * BRDF eval are approximated by 1.
     */
    const double radFraction = 0.03;

    double maxr2 = (static_cast<double>(*m_estimate_nrp) * Statistics::instance().radiance.totalArea /
                    (M_PI * static_cast<double>(m_totalPaths) * radFraction));

    return maxr2;
}

// Precompute Irradiance
void
PhotonMap::photonPrecomputeIrradiance(Camera */*camera*/, IrrPhoton *photon) {
    ColorRgb irradiance;
    irradiance.clear();

    // Locate the nearest photons using a max radius limit
    Vector3D pos = photon->pos();
    m_nrpFound = doQuery(&pos);

    if ( m_nrpFound > 3 ) {
        // Construct irradiance estimate
        float maxDistance = m_distances[0];

        for ( int i = 0; i < m_nrpFound; i++ ) {
            if ( photon->Normal().dotProduct(m_photons[i]->dir()) > 0 ) {
                ColorRgb power = m_photons[i]->power();
                irradiance.add(irradiance, power);
            }
        }

        // Now we have incoming radiance integrated over area estimate,
        // so we convert it to irradiance, maxDistance is already squared
        // An extra factor PI is added, that accounts for Albedo -> diffuse brdf...
        float factor = (1.0f / (static_cast<float>(M_PI) * static_cast<float>(M_PI) * maxDistance * static_cast<float>(m_totalPaths)));
        irradiance.scale(factor);
    }

    photon->SetIrradiance(irradiance);
}

void
PhotonMap::precomputeIrradianceCallback(void *data, void *nodeData) {
    PhotonMap *map = static_cast<PhotonMap *>(data);
    IrrPhoton *photon = static_cast<IrrPhoton *>(nodeData);
    map->photonPrecomputeIrradiance(nullptr, photon);
}

void
PhotonMap::precomputeIrradiance() {
    java::System::err.printf("PhotonMap::precomputeIrradiance\n");
    if ( m_precomputeIrradiance && !m_irradianceComputed ) {
        m_kdtree->iterateNodes(PhotonMap::precomputeIrradianceCallback, this);
        m_irradianceComputed = true;
    }
}

bool
PhotonMap::irradianceReconstruct(
    RayHit *hit,
    const Vector3D &/*outDir*/,
    const ColorRgb &diffuseAlbedo,
    ColorRgb *result)
{
    if ( !m_irradianceComputed ) {
        precomputeIrradiance();
    }

    Vector3D normal = hit->getNormal();
    Vector3D position = hit->getPoint();
    const IrrPhoton *photon = DoIrradianceQuery(&position, &normal);
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
PhotonMap::reconstruct(
    RayHit *hit,
    Vector3D &outDir,
    PhongBidirectionalScatteringDistributionFunction *bsdf,
    PhongBidirectionalScatteringDistributionFunction *inBsdf,
    PhongBidirectionalScatteringDistributionFunction *outBsdf)
{
    // Find the nearest photons
    ColorRgb result;
    ColorRgb eval;
    ColorRgb col;

    result.clear();

    ColorRgb diffuseAlbedo;
    ColorRgb glossyAlbedo;

    diffuseAlbedo.clear();
    glossyAlbedo.clear();

    if ( bsdf != nullptr ) {
        diffuseAlbedo = bsdf->splitBsdfScatteredPower(hit, BRDF_DIFFUSE_COMPONENT);
        // -- TODO Irradiance pre-computation for diffuse transmission
        glossyAlbedo = bsdf->splitBsdfScatteredPower(hit, BTDF_DIFFUSE_COMPONENT | BsdfComponentInfo::BSDF_GLOSSY_COMPONENT);
    }

    checkNBalance();

    if ( glossyAlbedo.average() < Numeric::EPSILON ) {
        if ( diffuseAlbedo.average() < Numeric::EPSILON ) {
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
    float maxDistance = m_distances[0];

    for ( int i = 0; i < m_nrpFound; i++ ) {
        Vector3D dir = m_photons[i]->dir();

        if ( bsdf == nullptr ) {
            eval.clear();
        } else {
            eval = bsdf->evaluate(
                hit, inBsdf, outBsdf, &outDir, &dir, BsdfComponentInfo::BSDF_DIFFUSE_COMPONENT | BsdfComponentInfo::BSDF_GLOSSY_COMPONENT);
        }
        ColorRgb power = m_photons[i]->power();

        col.scalarProduct(eval, power);
        result.add(result, col);
    }

    // Now we have a radiance integrated over area estimate,
    // so we convert it to radiance, maxDistance is already squared

    float factor = 1.0f / (static_cast<float>(M_PI) * maxDistance * static_cast<float>(m_totalPaths));

    result.scale(factor);

    return result;
}

float
PhotonMap::getCurrentDensity(RayHit &hit, int nrPhotons) {
    // Find the nearest photons
    if ( nrPhotons == 0 ) {
        nrPhotons = *m_estimate_nrp;
    }

    if ( nrPhotons == 0 ) {
        return 0.0;
    }

    Vector3D position = hit.getPoint();
    m_nrpFound = doQuery(&position, nrPhotons, static_cast<float>(GetMaxR2()));

    if ( m_nrpFound < 3 ) {
        return 0.0;
    }

    // Construct density estimate
    float maxDistance = m_distances[0]; // Only valid since max heap is used in kdtree

    computeCosines(hit.getGeometricNormal()); // Shading normal?

    if ( m_nrpCosinePos <= 3 ) {
        return 0.0;
    }

    return static_cast<float>(m_nrpCosinePos / (M_PI * maxDistance));
}

/**
Return a color coded density of the photon map
*/
ColorRgb
PhotonMap::getDensityColor(RayHit &hit) {
    float density = getCurrentDensity(hit, 0);

    ColorRgb result = PhotonMap::getFalseColor(density, photonMapState);

    return result;
}

double
PhotonMap::sample(
    Vector3D position,
    double *r,
    double *s,
    const CoordinateSystem *coord,
    char flag,
    float n)
{
    // -- Epsilon in as a function of scene/camera measure ??
    if ( !m_sampleLastPos.equals(position, 0.0001f) ) {
        // Need a new grid

        m_grid->init();

        // Find the nearest photons
        m_nrpFound = doQuery(&position, m_sample_nrp, KDTree::KD_MAX_RADIUS, NO_IMPSAMP_PHOTON);

        double pr;
        double ps;

        for ( int i = 0; i < m_nrpFound; i++ ) {
            // Section [ARVO1995b].2: each photon direction is re-parameterized
            // in a local spherical frame before building the 2D sampling grid.
            m_photons[i]->findRS(&pr, &ps, coord, flag, n);

            ColorRgb color = m_photons[i]->power();

            m_grid->add(pr, ps, color.average() / static_cast<float>(m_nrPhotons));
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

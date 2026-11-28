#include <stdlib.h>

#include "java/lang/Double.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "java/util/Formatter.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "java/lang/System.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/raycasting/photonMap/Photon.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMap.h"
bool
PhotonMap::zeroAlbedo(const PhongBidirScattDistFunc *bsdf, RpkRayHit *hit, char flags) {
    ColorRgb color;
    if ( bsdf == NULL ) {
        color = ColorRgb(0.0, 0.0, 0.0);
    } else {
        color = bsdf->splitBsdfScatteredPower(hit->shadingContext(NULL), flags);
    }
    return (ColorRgbMutable(color).average() < VSDK::EPSILON);
}

double
PhotonMap::getFalseMonochrome(double val, const PhotonMapState &photonMapState) {
    double max = photonMapState.falseColMax;

    if ( photonMapState.falseColLog ) {
        max = ((double)(Math::log(1.0 + max)));
        val = ((double)(Math::log(1.0 + val)));
    }

    double tmp = Math::min(val, max);
    tmp = (tmp / max);

    return tmp;
}

ColorRgb
PhotonMap::getFalseColor(double val, const PhotonMapState &photonMapState) {
    ColorRgb col;
    double tmp;
    double r = 0;
    double g = 0;
    double b = 0;

    if ( photonMapState.falseColMono ) {
        tmp = PhotonMap::getFalseMonochrome(val, photonMapState);
        col = ColorRgb(tmp, tmp, tmp);
        return col;
    }

    double max = photonMapState.falseColMax;

    if ( photonMapState.falseColLog ) {
        max = ((double)(Math::log(1.0 + max)));
        val = ((double)(Math::log(1.0 + val)));
    }

    tmp = Math::min(val, max);

    // Does some log scale ?

    tmp = 3.0 * (tmp / max);

    if ( tmp <= 1.0 ) {
        b = tmp;
    } else if ( tmp < 2.0 ) {
        g = tmp - 1.0;
        b = 1.0 - g;
    } else {
        r = tmp - 2.0;
        g = 1.0 - r;
    }

    col = ColorRgb(r, g, b);
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

    m_kdtree = new PhotonKDTree(doPrecomputeIrradiance);

    m_totalPaths = 0;
    m_nrPhotons = 0;
    m_totalPhotons = 0;

    m_grid = new SampleGrid2D(2, 4);
    m_sampleLastPos = Vector3Dd(Double::MAX_VALUE, Double::MAX_VALUE, Double::MAX_VALUE);

    m_photons = new Photon *[PHOTON_MAP_MAXIMUM_RECON_PHOTONS];
    m_distances = new double[PHOTON_MAP_MAXIMUM_RECON_PHOTONS];
    m_cosines = new double[PHOTON_MAP_MAXIMUM_RECON_PHOTONS];

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
PhotonMap::printStats(PrintStream *stream) const {
    if ( stream == NULL ) {
        return;
    }
    stream->printf("%i stored photons\n", m_nrPhotons);
}

void
PhotonMap::getStats(char *p, int n) const {
    Formatter::format(
        p, n, "%i stored photons, %i total, %li paths\n", m_nrPhotons, m_totalPhotons, m_totalPaths);
}

void
PhotonMap::computeCosines(const Vector3Dd normal) {
    if ( !m_cosinesOk ) {
        m_nrpCosinePos = 0;

        for ( int i = 0; i < m_nrpFound; i++ ) {
            Vector3Dd dir = m_photons[i]->dir();
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
    Vector3Dd normal,
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
PhotonMap::addPhoton(Photon &photon, Vector3Dd normal, short flags) {
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
    double currentD,
    double requiredD,
    const PhotonMapState &photonMapState) {
    // Step function
    if ( photonMapState.acceptPdfType == STEP ) {
        if ( currentD > requiredD ) {
            return 0.0;
        } else {
            return 1.0;
        }
    } else if ( photonMapState.acceptPdfType == TRANS_COSINE ) {
        // Translated cosine
        double ratio = Math::min(1.0, (double)(currentD / requiredD)); // in [0,1]

        return (0.5 * (1.0 + Math::cos(ratio * M_PI)));
    } else {
        Logger::reportMessage("PhotonMap", Logger::ERROR, "PhotonMap::computeAcceptProb", "Unknown accept pdf type");
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
    double factor = 1.0 / ((double)(m_nrpCosinePos));

    ColorRgb pow = photon.power();
    deltaPower = ColorRgb(factor * pow.getR(), factor * pow.getG(), factor * pow.getB());

    for ( int i = 0; i < m_nrpFound; i++ ) {
        if ( m_cosines[i] > 0.0 ) {
            m_photons[i]->addPower(deltaPower);
        }
    }
}

bool
PhotonMap::DC_AddPhoton(
    Photon &photon,
    RpkRayHit &hit,
    double requiredD,
    short flags)
{
    // Get current density
    // Vector3Dd pos = photon.Pos();
    bool stored;

    double currentD = getCurrentDensity(hit, photonMapState.distribPhotons);
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

    double maxr2 = (((double)(*m_estimate_nrp)) * Statistics::instance().radiance.totalArea /
                    (M_PI * ((double)(m_totalPaths)) * radFraction));

    return maxr2;
}

// Precompute Irradiance
void
PhotonMap::photonPrecomputeIrradiance(Camera */*camera*/, IrrPhoton *photon) {
    ColorRgb irradiance;
    irradiance = ColorRgb(0.0, 0.0, 0.0);

    // Locate the nearest photons using a max radius limit
    Vector3Dd pos = photon->pos();
    m_nrpFound = doQuery(&pos);

    if ( m_nrpFound > 3 ) {
        // Construct irradiance estimate
        double maxDistance = m_distances[0];

        for ( int i = 0; i < m_nrpFound; i++ ) {
            if ( photon->Normal().dotProduct(m_photons[i]->dir()) > 0 ) {
                ColorRgb power = m_photons[i]->power();
                irradiance = ColorRgb(
                    irradiance.getR() + power.getR(),
                    irradiance.getG() + power.getG(),
                    irradiance.getB() + power.getB());
            }
        }

        // Now we have incoming radiance integrated over area estimate,
        // so we convert it to irradiance, maxDistance is already squared
        // An extra factor PI is added, that accounts for Albedo -> diffuse brdf...
        double factor = (1.0 / (((double)(M_PI)) * ((double)(M_PI)) * maxDistance * ((double)(m_totalPaths))));
        irradiance = ColorRgb(
            factor * irradiance.getR(),
            factor * irradiance.getG(),
            factor * irradiance.getB());
    }

    photon->SetIrradiance(irradiance);
}

class PhotonIrradianceVisitor : public KDTreeVisitor<IrrPhoton> {
  private:
    PhotonMap &map;
  public:
    explicit PhotonIrradianceVisitor(PhotonMap &inMap): map(inMap) {}
    void visit(IrrPhoton &photon) { map.photonPrecomputeIrradiance(NULL, &photon); }
};

void
PhotonMap::precomputeIrradiance() {
    System::err.printf("PhotonMap::precomputeIrradiance\n");
    if ( m_precomputeIrradiance && !m_irradianceComputed ) {
        PhotonIrradianceVisitor visitor(*this);
        m_kdtree->iterateIrradiance(visitor);
        m_irradianceComputed = true;
    }
}

bool
PhotonMap::irradianceReconstruct(
    RpkRayHit *hit,
    const Vector3Dd &/*outDir*/,
    const ColorRgb &diffuseAlbedo,
    ColorRgb *result)
{
    if ( !m_irradianceComputed ) {
        precomputeIrradiance();
    }

    Vector3Dd normal = hit->getNormal();
    Vector3Dd position = hit->getPoint();
    const IrrPhoton *photon = DoIrradianceQuery(&position, &normal);
    hit->setNormal(&normal);

    if ( photon ) {
        *result = ColorRgb(
            photon->m_irradiance.getR() * diffuseAlbedo.getR(),
            photon->m_irradiance.getG() * diffuseAlbedo.getG(),
            photon->m_irradiance.getB() * diffuseAlbedo.getB());
        return true;
    } else {
        // No appropriate photon found
        return false;
    }
}

ColorRgb
PhotonMap::reconstruct(
    RpkRayHit *hit,
    Vector3Dd &outDir,
    PhongBidirScattDistFunc *bsdf,
    PhongBidirScattDistFunc *inBsdf,
    PhongBidirScattDistFunc *outBsdf)
{
    // Find the nearest photons
    ColorRgb result;
    ColorRgb eval;
    ColorRgb col;

    result = ColorRgb(0.0, 0.0, 0.0);

    ColorRgb diffuseAlbedo;
    ColorRgb glossyAlbedo;

    diffuseAlbedo = ColorRgb(0.0, 0.0, 0.0);
    glossyAlbedo = ColorRgb(0.0, 0.0, 0.0);

    if ( bsdf != NULL ) {
        diffuseAlbedo = bsdf->splitBsdfScatteredPower(hit->shadingContext(NULL), BRDF_DIFFUSE_COMPONENT);
        // -- TODO Irradiance pre-computation for diffuse transmission
        glossyAlbedo = bsdf->splitBsdfScatteredPower(hit->shadingContext(NULL), BTDF_DIFFUSE_COMPONENT | BsdfComponentInfo::BSDF_GLOSSY_COMPONENT);
    }

    checkNBalance();

    if ( ColorRgbMutable(glossyAlbedo).average() < VSDK::EPSILON ) {
        if ( ColorRgbMutable(diffuseAlbedo).average() < VSDK::EPSILON ) {
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
    Vector3Dd position = hit->getPoint();
    m_nrpFound = doQuery(&position);

    if ( m_nrpFound < 3 ) {
        return result;
    }

    // Construct radiance estimate
    double maxDistance = m_distances[0];
    ShadingContext context = hit->shadingContext(NULL);

    for ( int i = 0; i < m_nrpFound; i++ ) {
        Vector3Dd dir = m_photons[i]->dir();

        if ( bsdf == NULL ) {
            eval = ColorRgb(0.0, 0.0, 0.0);
        } else {
            eval = bsdf->evaluate(
                context, inBsdf, outBsdf, &outDir, &dir, BsdfComponentInfo::BSDF_DIFFUSE_COMPONENT | BsdfComponentInfo::BSDF_GLOSSY_COMPONENT);
        }
        ColorRgb power = m_photons[i]->power();

        col = ColorRgb(
            eval.getR() * power.getR(),
            eval.getG() * power.getG(),
            eval.getB() * power.getB());
        result = ColorRgb(
            result.getR() + col.getR(),
            result.getG() + col.getG(),
            result.getB() + col.getB());
    }

    // Now we have a radiance integrated over area estimate,
    // so we convert it to radiance, maxDistance is already squared

    double factor = 1.0 / (((double)(M_PI)) * maxDistance * ((double)(m_totalPaths)));

    result = ColorRgb(
        factor * result.getR(),
        factor * result.getG(),
        factor * result.getB());

    return result;
}

double
PhotonMap::getCurrentDensity(RpkRayHit &hit, int nrPhotons) {
    // Find the nearest photons
    if ( nrPhotons == 0 ) {
        nrPhotons = *m_estimate_nrp;
    }

    if ( nrPhotons == 0 ) {
        return 0.0;
    }

    Vector3Dd position = hit.getPoint();
    m_nrpFound = doQuery(&position, nrPhotons, ((double)(GetMaxR2())));

    if ( m_nrpFound < 3 ) {
        return 0.0;
    }

    // Construct density estimate
    double maxDistance = m_distances[0]; // Only valid since max heap is used in kdtree

    computeCosines(hit.getGeometricNormal()); // Shading normal?

    if ( m_nrpCosinePos <= 3 ) {
        return 0.0;
    }

    return ((double)(m_nrpCosinePos / (M_PI * maxDistance)));
}

/**
Return a color coded density of the photon map
*/
ColorRgb
PhotonMap::getDensityColor(RpkRayHit &hit) {
    double density = getCurrentDensity(hit, 0);

    ColorRgb result = PhotonMap::getFalseColor(density, photonMapState);

    return result;
}

double
PhotonMap::sample(
    Vector3Dd position,
    double *r,
    double *s,
    const CoordinateSystem *coord,
    char flag,
    double n)
{
    // -- Epsilon in as a function of scene/camera measure ??
    if ( !Numeric::vectorsEqual(m_sampleLastPos, position, 0.0001) ) {
        // Need a new grid

        m_grid->init();

        // Find the nearest photons
        m_nrpFound = doQuery(&position, m_sample_nrp, KDTree<Photon>::KD_MAX_RADIUS, NO_IMPSAMP_PHOTON);

        double pr;
        double ps;

        for ( int i = 0; i < m_nrpFound; i++ ) {
            // Section [ARVO1995b].2: each photon direction is re-parameterized
            // in a local spherical frame before building the 2D sampling grid.
            m_photons[i]->findRS(&pr, &ps, coord, flag, n);

            ColorRgb color = m_photons[i]->power();

            m_grid->add(pr, ps, ColorRgbMutable(color).average() / ((double)(m_nrPhotons)));
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

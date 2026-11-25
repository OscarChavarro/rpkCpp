#include <cstdlib>

#include "java/lang/Float.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "java/util/Formatter.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "java/lang/System.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/raycasting/photonMap/Photon.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMap.h"
bool
PhotonMap::zeroAlbedo(const PhongBidirectionalScatteringDistributionFunction *bsdf, RpkRayHit *hit, char flags) {
    bool shctxOk = false;
    ShadingContext shctx = hit->shadingContext(&shctxOk);
    if ( !shctxOk || bsdf == nullptr ) {
        return true;
    } else {
        const ColorRgb color = bsdf->splitBsdfScatteredPower(shctx, flags);
        return color.average() < VSDK::EPSILON;
    }
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
    float tmp;
    float r = 0;
    float g = 0;
    float b = 0;

    if ( photonMapState.falseColMono ) {
        tmp = PhotonMap::getFalseMonochrome(val, photonMapState);
        return ColorRgb(tmp, tmp, tmp);
    }

    float max = photonMapState.falseColMax;

    if ( photonMapState.falseColLog ) {
        max = static_cast<float>(java::Math::log(1.0 + max));
        val = static_cast<float>(java::Math::log(1.0 + val));
    }

    tmp = java::Math::min(val, max);

    // Does some log scale ?

    tmp = 3.0F * (tmp / max);

    if ( tmp <= 1.0 ) {
        b = tmp;
    } else if ( tmp < 2.0 ) {
        g = tmp - 1.0F;
        b = 1.0F - g;
    } else {
        r = tmp - 2.0F;
        g = 1.0F - r;
    }

    return ColorRgb(r, g, b);
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
    m_sampleLastPos = Vector3Df(java::Float::MAX_VALUE, java::Float::MAX_VALUE, java::Float::MAX_VALUE);

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
PhotonMap::computeCosines(const Vector3Df normal) {
    if ( !m_cosinesOk ) {
        m_nrpCosinePos = 0;

        for ( int i = 0; i < m_nrpFound; i++ ) {
            Vector3Df dir = m_photons[i]->dir();
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
    Vector3Df normal,
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
PhotonMap::addPhoton(Photon &photon, Vector3Df normal, short flags) {
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
        double ratio = java::Math::min(1.0, static_cast<double>(currentD / requiredD)); // in [0,1]

        return (0.5 * (1.0 + java::Math::cos(ratio * M_PI)));
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

    float factor = 1.0F / static_cast<float>(m_nrpCosinePos);
    const ColorRgb deltaPower = photon.power().multiply(factor);

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
    float requiredD,
    short flags)
{
    // Get current density
    // Vector3Df pos = photon.Pos();
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
    ColorRgb irradiance(0.0, 0.0, 0.0);

    // Locate the nearest photons using a max radius limit
    Vector3Df pos = photon->pos();
    m_nrpFound = doQuery(&pos);

    if ( m_nrpFound > 3 ) {
        // Construct irradiance estimate
        float maxDistance = m_distances[0];

        for ( int i = 0; i < m_nrpFound; i++ ) {
            if ( photon->Normal().dotProduct(m_photons[i]->dir()) > 0 ) {
                irradiance = irradiance.add(m_photons[i]->power());
            }
        }

        // Now we have incoming radiance integrated over area estimate,
        // so we convert it to irradiance, maxDistance is already squared
        // An extra factor PI is added, that accounts for Albedo -> diffuse brdf...
        float factor = (1.0F / (static_cast<float>(M_PI) * static_cast<float>(M_PI) * maxDistance * static_cast<float>(m_totalPaths)));
        irradiance = irradiance.multiply(factor);
    }

    photon->SetIrradiance(irradiance);
}

void
PhotonMap::precomputeIrradiance() {
    java::System::err.printf("PhotonMap::precomputeIrradiance\n");
    if ( m_precomputeIrradiance && !m_irradianceComputed ) {
        class IrradianceVisitor {
          private:
            PhotonMap &map;
          public:
            explicit IrradianceVisitor(PhotonMap &inMap): map(inMap) {}
            void visit(IrrPhoton &photon) { map.photonPrecomputeIrradiance(nullptr, &photon); }
        } visitor(*this);
        m_kdtree->iterateIrradiance(visitor);
        m_irradianceComputed = true;
    }
}

bool
PhotonMap::irradianceReconstruct(
    RpkRayHit *hit,
    const Vector3Df &/*outDir*/,
    const ColorRgb &diffuseAlbedo,
    ColorRgb *result)
{
    if ( !m_irradianceComputed ) {
        precomputeIrradiance();
    }

    Vector3Df normal = hit->getNormal();
    Vector3Df position = hit->getPoint();
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
    Vector3Df &outDir,
    PhongBidirectionalScatteringDistributionFunction *bsdf,
    PhongBidirectionalScatteringDistributionFunction *inBsdf,
    PhongBidirectionalScatteringDistributionFunction *outBsdf)
{
    // Find the nearest photons
    double resultR = 0.0;
    double resultG = 0.0;
    double resultB = 0.0;
    ColorRgb diffuseAlbedo(0.0, 0.0, 0.0);
    ColorRgb glossyAlbedo(0.0, 0.0, 0.0);

    bool shctxOk = false;
    ShadingContext shctx = hit->shadingContext(&shctxOk);
    if ( bsdf != nullptr && shctxOk ) {
        diffuseAlbedo = bsdf->splitBsdfScatteredPower(shctx, BRDF_DIFFUSE_COMPONENT);
        // -- TODO Irradiance pre-computation for diffuse transmission
        glossyAlbedo = bsdf->splitBsdfScatteredPower(shctx, BTDF_DIFFUSE_COMPONENT | BsdfComponentInfo::BSDF_GLOSSY_COMPONENT);
    }

    checkNBalance();

    if ( glossyAlbedo.average() < VSDK::EPSILON ) {
        if ( diffuseAlbedo.average() < VSDK::EPSILON ) {
            return ColorRgb(resultR, resultG, resultB); // No reflectance
        } else {
            if ( m_precomputeIrradiance ) {
                ColorRgb irradianceResult(0.0, 0.0, 0.0);
                if ( irradianceReconstruct(hit, outDir, diffuseAlbedo, &irradianceResult) ) {
                    return irradianceResult;
                } else {
                    // No appropriate irradiance photon -> do normal reconstruction
                }
            }
        }
    }

    // Normal reconstruct...

    // Locate nearest photons using a max radius limit
    Vector3Df position = hit->getPoint();
    m_nrpFound = doQuery(&position);

    if ( m_nrpFound < 3 ) {
        return ColorRgb(resultR, resultG, resultB);
    }

    // Construct radiance estimate
    float maxDistance = m_distances[0];

    for ( int i = 0; i < m_nrpFound; i++ ) {
        Vector3Df dir = m_photons[i]->dir();

        ColorRgb eval(0.0, 0.0, 0.0);
        if ( bsdf == nullptr || !shctxOk ) {
            eval = ColorRgb(0.0, 0.0, 0.0);
        } else {
            eval = bsdf->evaluate(
                shctx, inBsdf, outBsdf, &outDir, &dir, BsdfComponentInfo::BSDF_DIFFUSE_COMPONENT | BsdfComponentInfo::BSDF_GLOSSY_COMPONENT);
        }
        const ColorRgb power = m_photons[i]->power();
        const double colR = eval.getR() * power.getR();
        const double colG = eval.getG() * power.getG();
        const double colB = eval.getB() * power.getB();
        resultR = resultR + colR;
        resultG = resultG + colG;
        resultB = resultB + colB;
    }

    // Now we have a radiance integrated over area estimate,
    // so we convert it to radiance, maxDistance is already squared

    float factor = 1.0F / (static_cast<float>(M_PI) * maxDistance * static_cast<float>(m_totalPaths));

    return ColorRgb(resultR * factor, resultG * factor, resultB * factor);
}

float
PhotonMap::getCurrentDensity(RpkRayHit &hit, int nrPhotons) {
    // Find the nearest photons
    if ( nrPhotons == 0 ) {
        nrPhotons = *m_estimate_nrp;
    }

    if ( nrPhotons == 0 ) {
        return 0.0;
    }

    Vector3Df position = hit.getPoint();
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
PhotonMap::getDensityColor(RpkRayHit &hit) {
    float density = getCurrentDensity(hit, 0);

    return PhotonMap::getFalseColor(density, photonMapState);
}

double
PhotonMap::sample(
    Vector3Df position,
    double *r,
    double *s,
    const CoordinateSystem *coord,
    char flag,
    float n)
{
    // -- Epsilon in as a function of scene/camera measure ??
    if ( !Numeric::vectorsEqual(m_sampleLastPos, position, 0.0001F) ) {
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

            const ColorRgb color = m_photons[i]->power();

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

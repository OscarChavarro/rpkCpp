/**
Implementation of the special importance map functions
*/
#include "java/lang/System.h"
#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "photonMap/ImportanceMap.h"
#include "photonMap/Importon.h"
#include "common/Error.h"

bool
ImportanceMap::addPhoton(
    Photon &photon,
    Vector3D normal,
    short flags)
{
    return PhotonMap::addPhoton(photon, normal, flags);
}

// reconstruct
float
ImportanceMap::reconstructImportance(Vector3D /*pos*/, const Vector3D &normal) const  {
    float maxDistance;
    float result = 0.0;
    float importance;
    float factor;

    // Nearest photons must be found beforehand!

    // Construct radiance estimate
    maxDistance = m_distances[0];

    for ( int i = 0; i < m_nrpFound; i++ ) {
        const Importon *importon = static_cast<Importon *>(m_photons[i]);

        Vector3D dir = importon->dir();

        // No bsdf eval : incoming importance !!!!!!
        importance = importon->Importance();

        float cos_theta = dir.dotProduct(normal);
        if ( cos_theta > 0.0 ) {
            result += importance;
        }
    }

    // Now we have a 'importance' integrated over area estimate,
    // so we convert it to 'importance/potential', maxDistance is already squared

    if ( maxDistance < 1e-5 ) {
        return 0;
    }

    factor = 1.0f / (static_cast<float>(M_PI) * maxDistance * static_cast<float>(m_totalPaths));
    result *= factor;

    return result;
}

float
ImportanceMap::getImpReqDensity(const Camera *camera, const Vector3D &pos, const Vector3D &normal) const {
    // reconstruct importance
    float density = reconstructImportance(pos, normal);

    // We want impScale photons per pixel density, account for
    // the pixel area here

    density /= camera->pixelWidth * camera->pixelHeight;

    return density;
}

float
ImportanceMap::getRequiredDensity(const Camera *camera, Vector3D pos, Vector3D normal) {
    if ( m_nrPhotons == 0 ) {
        return photonMapState.constantRD;
    }  // Safety, if no importance map was constructed

    float density;

    checkNBalance();

    if ( m_precomputeIrradiance ) {
        if ( !m_irradianceComputed || (m_preReconPhotons != *m_estimate_nrp))
            precomputeIrradiance();

        const Importon *photon = static_cast<Importon *>(DoIrradianceQuery(&pos, &normal, m_totalMaxDistance));

        if ( photon ) {
            switch ( photonMapState.importanceOption ) {
                case USE_IMPORTANCE:
                    density = photon->PImportance();
                    density *= *m_impScalePtr;
                    break;
                default:
                    Error::error("ImportanceMap::getRequiredDensity", "Unsupported importance option");
                    return 0;
            }
        } else
            density = 0;
    } else {
        // normal query or no irradiance photon found

        // Query photons, to be used by the appropriate req dest method
        m_nrpFound = doQuery(&pos);

        if ( m_nrpFound < 3 )
            return 0; // State for minimumImpRD

        switch ( photonMapState.importanceOption ) {
            case USE_IMPORTANCE:
                density = getImpReqDensity(camera, pos, normal);
                density *= *m_impScalePtr;
                break;
            default:
                Error::error("ImportanceMap::getRequiredDensity", "Unsupported importance option");
                return 0;
        }
    }

    // Minimum required density
    if ( density < photonMapState.minimumImpRD )
        density = photonMapState.minimumImpRD;
    return density;
}

void
ImportanceMap::ComputeAllRequiredDensities(
    const Camera *camera,
    Vector3D &pos,
    const Vector3D &normal,
    float *imp,
    float *pot,
    float *diff)
{
    // Query photons, to be used by the appropriate req dest method
    m_nrpFound = doQuery(&pos);
    if ( m_nrpFound < 5 ) {
        // not enough photons
        *imp = *pot = *diff = 0.0;
    }

    *imp = getImpReqDensity(camera, pos, normal);
}

void
ImportanceMap::photonPrecomputeIrradiance(Camera *camera, IrrPhoton *photon) {
    float imp;
    float pot;
    float diff{};
    Vector3D pos = photon->pos();
    Vector3D normal = photon->Normal();

    ComputeAllRequiredDensities(camera, pos, normal, &imp, &pot, &diff);

    // Abuse pot for tail enhancement
    pot = m_distances[0]; // Only valid since max heap is used in kd-tree
    m_totalMaxDistance = java::Math::max(pot, m_totalMaxDistance);

    static_cast<Importon *>(photon)->PSetAll(imp, pot, diff);
    if ( imp > m_maxImp ) {
        m_maxImp = imp;
    }

    m_avgImp += imp;
}

void
ImportanceMap::precomputeIrradiance() {
    java::System::err.printf("ImportanceMap::precomputeIrradiance\n");
    m_maxImp = 0;
    m_avgImp = 0;
    m_preReconPhotons = *m_estimate_nrp;
    m_totalMaxDistance = 0.0;
    m_irradianceComputed = false;

    PhotonMap::precomputeIrradiance();

    m_avgImp /= static_cast<float>(m_nrPhotons);
    m_totalMaxDistance *= 20.0f / static_cast<float>(*m_estimate_nrp);
}

#endif

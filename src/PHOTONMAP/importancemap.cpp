/* 
 * Implementation of the special importance map functions
 */

#include "material/statistics.h"
#include "PHOTONMAP/pmapoptions.h"
#include "PHOTONMAP/importancemap.h"
#include "common/error.h"

bool CImportanceMap::AddPhoton(CPhoton &photon, Vector3D &normal,
                               short flags) {

    return CPhotonMap::AddPhoton(photon, normal, flags);
}

// Reconstruct
float CImportanceMap::ReconstructImp(Vector3D &pos, Vector3D &normal /*, IMPORTANCE_OPTION option */) {
    float maxDistance = 0.0;
    float result = 0., importance;
    float factor;

    // Nearest photons must be found beforehand!

    // Construct radiance estimate
    maxDistance = m_distances[0];

    for ( int i = 0; i < m_nrpFound; i++ ) {
        CImporton *importon = (CImporton *) m_photons[i];

        Vector3D dir = importon->Dir();

        // No bsdf eval : incoming importance !!!!!!
        importance = importon->Importance();

        float cos_theta = VECTORDOTPRODUCT(dir, normal);
        if ( cos_theta > 0.0 ) {
            result += importance;
        }
    }

    // Now we have a 'importance' integrated over area estimate,
    // so we convert it to 'importance/potential', maxDistance is already squared

    if ( maxDistance < 1e-5 ) {
        return 0;
    }

    factor = 1.0 / (M_PI * maxDistance * m_totalPaths);
    result *= factor;

    return result;
}


float CImportanceMap::GetImpReqDensity(Vector3D &pos, Vector3D &normal) {
    float density;

    // Reconstruct importance
    density = ReconstructImp(pos, normal);

    // Rescale
    //if(!pmapstate.pixelImportance)
    //  density *= GLOBAL_camera_mainCamera.hres * GLOBAL_camera_mainCamera.vres;

    // We want impScale photons per pixel density, account for
    // the pixel area here

    density /= GLOBAL_camera_mainCamera.pixh * GLOBAL_camera_mainCamera.pixv;

    return (density);
}


float CImportanceMap::GetRequiredDensity(Vector3D &pos, Vector3D &normal) {
    if ( m_nrPhotons == 0 ) {
        return pmapstate.constantRD;
    }  // Safety, if no importance map was constructed

    float density;

    CheckNBalance();

    if ( m_precomputeIrradiance ) {
        if ( !m_irradianceComputed || (m_preReconPhotons != *m_estimate_nrp))
            PrecomputeIrradiance();

        CImporton *photon = (CImporton *) DoIrradianceQuery(&pos, &normal,
                                                            m_totalMaxDistance);

        if ( photon ) {
            switch ( pmapstate.importanceOption ) {
                case USE_IMPORTANCE:
                    density = photon->PImportance();
                    density *= *m_impScalePtr;
                    break;
                default:
                    Error("CImportanceMap::GetRequiredDensity", "Unsupported importance option");
                    return 0;
                    break;
            }
        } else
            density = 0;
    } else {
        // normal query or no irradiance photon found

        // Query photons, to be used by the appropriate req dest method
        m_nrpFound = DoQuery(&pos);

        if ( m_nrpFound < 3 )
            return 0; // pmapstate.minimumImpRD;

        switch ( pmapstate.importanceOption ) {
            case USE_IMPORTANCE:
                density = GetImpReqDensity(pos, normal);
                density *= *m_impScalePtr;
                break;
            default:
                Error("CImportanceMap::GetRequiredDensity", "Unsupported importance option");
                return 0;
                break;
        }
    }

    // Minimum required density
    if ( density < pmapstate.minimumImpRD )
        density = pmapstate.minimumImpRD;
    return density;
}


void CImportanceMap::ComputeAllRequiredDensities(Vector3D &pos, Vector3D &normal,
                                                 float *imp, float *pot, float *diff) {
    // Query photons, to be used by the appropriate req dest method
    m_nrpFound = DoQuery(&pos);
    if ( m_nrpFound < 5 ) {
        // not enough photons
        *imp = *pot = *diff = 0.0;
    }

    *imp = GetImpReqDensity(pos, normal);
}


void CImportanceMap::PhotonPrecomputeIrradiance(CIrrPhoton *photon) {
    float imp, pot, diff;
    Vector3D pos = photon->Pos();
    Vector3D normal = photon->Normal();

    ComputeAllRequiredDensities(pos, normal, &imp, &pot, &diff);

    // Abuse pot for tail enhancement
    pot = m_distances[0]; // Only valid since max heap is used in kdtree
    m_totalMaxDistance = MAX(pot, m_totalMaxDistance);

    ((CImporton *) photon)->PSetAll(imp, pot, diff);
    if ( imp > m_maxImp ) {
        m_maxImp = imp;
    }


    m_avgImp += imp;

}

void CImportanceMap::PrecomputeIrradiance() {
    fprintf(stderr, "CImportanceMap::PrecomputeIrradiance\n");
    m_maxImp = 0;
    m_avgImp = 0;


    m_preReconPhotons = *m_estimate_nrp;
    m_totalMaxDistance = 0.0;
    m_irradianceComputed = false;

    // fprintf(stderr, "MSDiff %g, AVGSDIFF %g\n", m_maxSingleDiff,
    // m_sumSingleDiff/ m_nrPhotons);

    CPhotonMap::PrecomputeIrradiance();

    // fprintf(stderr, "MaxImp %g, MaxDiff %g\n", m_maxImp, m_maxDiff);

    m_avgImp /= m_nrPhotons;

    m_totalMaxDistance *= 20.0 / *m_estimate_nrp;

    // fprintf(stderr, "MaxImp %g, MaxDiff %g\n", m_maxImp, m_maxDiff);
    // fprintf(stderr, "AvgImp %g, AvgDiff %g\n", m_avgImp, m_avgDiff);
}



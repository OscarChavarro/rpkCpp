/**
The real importance map storage
*/

package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.scene.Camera;

public class ImportanceMap extends PhotonMap {
    private float m_maxImp;
    private float m_avgImp;
    private float m_totalMaxDistance;
    private int m_preReconPhotons;
    private float[] m_impScalePtr;

    // Constructor: always use irradiance per computation
    public ImportanceMap(PhotonMapState photonMapState, int[] estimate_nrp, float[] impScalePtr) {
        super(photonMapState, estimate_nrp, true);
        m_maxImp = 0.0f;
        m_avgImp = 0.0f;
        m_totalMaxDistance = 0.0f;
        m_preReconPhotons = 0;
        m_impScalePtr = impScalePtr;
    }

    // Override some photon map functions
    @Override
    public boolean addPhoton(Photon photon, Vector3D normal, short flags) {
        return super.addPhoton(photon, normal, flags);
    }

    // reconstruct
    public float reconstructImportance(Vector3D pos, Vector3D normal) {
        float maxDistance;
        float result = 0.0f;
        float importance;
        float factor;

        // Nearest photons must be found beforehand!

        // Construct radiance estimate
        maxDistance = m_distances[0];

        for ( int i = 0; i < m_nrpFound; i++ ) {
            Importon importon = (Importon)m_photons[i];

            Vector3D dir = importon.dir();

            // No bsdf eval : incoming importance !!!!!!
            importance = importon.Importance();

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

        factor = 1.0f / ((float)Math.PI * maxDistance * (float)m_totalPaths);
        result *= factor;

        return result;
    }

    public float getImpReqDensity(Camera camera, Vector3D pos, Vector3D normal) {
        // reconstruct importance
        float density = reconstructImportance(pos, normal);

        // We want impScale photons per pixel density, account for
        // the pixel area here

        density /= camera.pixelWidth * camera.pixelHeight;

        return density;
    }

    public float getRequiredDensity(Camera camera, Vector3D pos, Vector3D normal) {
        if ( m_nrPhotons == 0 ) {
            return photonMapState.constantRD;
        }  // Safety, if no importance map was constructed

        float density;

        checkNBalance();

        if ( m_precomputeIrradiance ) {
            if ( !m_irradianceComputed || (m_preReconPhotons != m_estimate_nrp[0])) {
                precomputeIrradiance();
            }

            Importon photon = (Importon)DoIrradianceQuery(pos, normal, m_totalMaxDistance);

            if ( photon != null ) {
                switch ( photonMapState.importanceOption ) {
                    case PhotonMapImportanceOptions.USE_IMPORTANCE:
                        density = photon.PImportance();
                        density *= m_impScalePtr[0];
                        break;
                    default:
                        Error.error("ImportanceMap::getRequiredDensity", "Unsupported importance option");
                        return 0;
                }
            } else {
                density = 0;
            }
        } else {
            // normal query or no irradiance photon found

            // Query photons, to be used by the appropriate req dest method
            m_nrpFound = doQuery(pos);

            if ( m_nrpFound < 3 ) {
                return 0; // State for minimumImpRD
            }

            switch ( photonMapState.importanceOption ) {
                case PhotonMapImportanceOptions.USE_IMPORTANCE:
                    density = getImpReqDensity(camera, pos, normal);
                    density *= m_impScalePtr[0];
                    break;
                default:
                    Error.error("ImportanceMap::getRequiredDensity", "Unsupported importance option");
                    return 0;
            }
        }

        // Minimum required density
        if ( density < photonMapState.minimumImpRD ) {
            density = photonMapState.minimumImpRD;
        }
        return density;
    }

    protected void
    ComputeAllRequiredDensities(
        Camera camera,
        Vector3D pos,
        Vector3D normal,
        float[] imp,
        float[] pot,
        float[] diff)
    {
        // Query photons, to be used by the appropriate req dest method
        m_nrpFound = doQuery(pos);
        if ( m_nrpFound < 5 ) {
            // not enough photons
            imp[0] = 0.0f;
            pot[0] = 0.0f;
            diff[0] = 0.0f;
            return;
        }

        imp[0] = getImpReqDensity(camera, pos, normal);
    }

    @Override
    public void photonPrecomputeIrradiance(Camera camera, IrrPhoton photon) {
        float[] imp = new float[1];
        float[] pot = new float[1];
        float[] diff = new float[1];
        Vector3D pos = photon.pos();
        Vector3D normal = photon.Normal();

        ComputeAllRequiredDensities(camera, pos, normal, imp, pot, diff);

        // Abuse pot for tail enhancement
        pot[0] = m_distances[0]; // Only valid since max heap is used in kd-tree
        m_totalMaxDistance = Math.max(pot[0], m_totalMaxDistance);

        ((Importon)photon).PSetAll(imp[0], pot[0], diff[0]);
        if ( imp[0] > m_maxImp ) {
            m_maxImp = imp[0];
        }

        m_avgImp += imp[0];
    }

    @Override
    public void precomputeIrradiance() {
        System.err.printf("ImportanceMap::precomputeIrradiance\n");
        m_maxImp = 0;
        m_avgImp = 0;
        m_preReconPhotons = m_estimate_nrp[0];
        m_totalMaxDistance = 0.0f;
        m_irradianceComputed = false;

        super.precomputeIrradiance();

        if ( m_nrPhotons > 0 ) {
            m_avgImp /= (float)m_nrPhotons;
        }
        if ( m_estimate_nrp[0] > 0 ) {
            m_totalMaxDistance *= 20.0f / (float)m_estimate_nrp[0];
        }
    }
}

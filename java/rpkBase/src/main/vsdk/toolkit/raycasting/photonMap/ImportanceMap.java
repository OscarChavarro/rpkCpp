/**
The real importance map storage
*/

package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.color.ColorRgb;

import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.scene.Camera;

public class ImportanceMap extends PhotonMap {
    private double m_maxImp;
    private double m_avgImp;
    private double m_totalMaxDistance;
    private int m_preReconPhotons;
    private double[] m_impScalePtr;

    // Constructor: always use irradiance per computation
    public ImportanceMap(PhotonMapState photonMapState, int[] estimate_nrp, double[] impScalePtr) {
        super(photonMapState, estimate_nrp, true);
        m_maxImp = 0.0;
        m_avgImp = 0.0;
        m_totalMaxDistance = 0.0;
        m_preReconPhotons = 0;
        m_impScalePtr = impScalePtr;
    }

    // Override some photon map functions
    @Override
    public boolean addPhoton(Photon photon, Vector3Dd normal, short flags) {
        return super.addPhoton(photon, normal, flags);
    }

    // reconstruct
    public double reconstructImportance(Vector3Dd pos, Vector3Dd normal) {
        double maxDistance;
        double result = 0.0;
        double importance;
        double factor;

        // Nearest photons must be found beforehand!

        // Construct radiance estimate
        maxDistance = m_distances[0];

        for ( int i = 0; i < m_nrpFound; i++ ) {
            Photon importon = m_photons[i];

            Vector3Dd dir = importon.dir();

            // No bsdf eval : incoming importance !!!!!!
            importance = (double)importon.power().getR();

            double cos_theta = dir.dotProduct(normal);
            if ( cos_theta > 0.0 ) {
                result += importance;
            }
        }

        // Now we have a 'importance' integrated over area estimate,
        // so we convert it to 'importance/potential', maxDistance is already squared

        if ( maxDistance < 1e-5 ) {
            return 0;
        }

        factor = 1.0 / ((double)Math.PI * maxDistance * (double)m_totalPaths);
        result *= factor;

        return result;
    }

    public double getImpReqDensity(Camera camera, Vector3Dd pos, Vector3Dd normal) {
        // reconstruct importance
        double density = reconstructImportance(pos, normal);

        // We want impScale photons per pixel density, account for
        // the pixel area here

        density /= camera.pixelWidth * camera.pixelHeight;

        return density;
    }

    public double getRequiredDensity(Camera camera, Vector3Dd pos, Vector3Dd normal) {
        if ( m_nrPhotons == 0 ) {
            return photonMapState.constantRD;
        }  // Safety, if no importance map was constructed

        double density;

        checkNBalance();

        if ( m_precomputeIrradiance ) {
            if ( !m_irradianceComputed || (m_preReconPhotons != m_estimate_nrp[0])) {
                precomputeIrradiance();
            }

            IrrPhoton photon = DoIrradianceQuery(pos, normal, m_totalMaxDistance);

            if ( photon != null ) {
                switch ( photonMapState.importanceOption ) {
                    case PhotonMapImportanceOptions.USE_IMPORTANCE:
                        density = (double)photon.m_irradiance.getR();
                        density *= m_impScalePtr[0];
                        break;
                    default:
                        Logger.reportMessage(this, Logger.ERROR, "ImportanceMap::getRequiredDensity", "Unsupported importance option");
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
                    Logger.reportMessage(this, Logger.ERROR, "ImportanceMap::getRequiredDensity", "Unsupported importance option");
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
        Vector3Dd pos,
        Vector3Dd normal,
        double[] imp,
        double[] pot,
        double[] diff)
    {
        // Query photons, to be used by the appropriate req dest method
        m_nrpFound = doQuery(pos);
        if ( m_nrpFound < 5 ) {
            // not enough photons
            imp[0] = 0.0;
            pot[0] = 0.0;
            diff[0] = 0.0;
            return;
        }

        imp[0] = getImpReqDensity(camera, pos, normal);
    }

    @Override
    public void photonPrecomputeIrradiance(Camera camera, IrrPhoton photon) {
        double[] imp = new double[1];
        double[] pot = new double[1];
        double[] diff = new double[1];
        Vector3Dd pos = photon.pos();
        Vector3Dd normal = photon.Normal();

        ComputeAllRequiredDensities(camera, pos, normal, imp, pot, diff);

        // Abuse pot for tail enhancement
        pot[0] = m_distances[0]; // Only valid since max heap is used in kd-tree
        m_totalMaxDistance = Math.max(pot[0], m_totalMaxDistance);

        photon.SetIrradiance(new vsdk.toolkit.common.color.ColorRgb(imp[0], imp[0], imp[0]));
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
        m_totalMaxDistance = 0.0;
        m_irradianceComputed = false;

        super.precomputeIrradiance();

        if ( m_nrPhotons > 0 ) {
            m_avgImp /= (double)m_nrPhotons;
        }
        if ( m_estimate_nrp[0] > 0 ) {
            m_totalMaxDistance *= 20.0 / (double)m_estimate_nrp[0];
        }
    }
}

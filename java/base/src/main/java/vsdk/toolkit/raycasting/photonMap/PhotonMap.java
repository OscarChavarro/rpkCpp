package vsdk.toolkit.raycasting.photonMap;

import java.io.PrintStream;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.PhongBidirectionalScatteringDistributionFunction;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.environment.geometry.elements.RayHit;

public class PhotonMap {
    protected PhotonMapState photonMapState;
    protected boolean m_balanced;
    protected boolean m_doBalancing;

    protected boolean m_precomputeIrradiance;
    protected boolean m_irradianceComputed;

    // kdtree storage

    protected int[] m_estimate_nrp; // Points to GUI changeable value (comfort)
    protected int m_sample_nrp;
    protected int m_nrPhotons;
    protected int m_totalPhotons;
    protected long m_totalPaths; // Number of traced paths, not number of photons!!
    // Stored flux value still has to be divided by the total number of paths.

    protected PhotonKDTree m_kdtree;

    // A grid is permanently allocated for importance sampling
    protected SampleGrid2D m_grid;
    protected Vector3D m_sampleLastPos;

    // Space to hold the photons and distances for queries

    protected int m_nrpFound; // Number of photons in the array
    protected int m_nrpCosinePos; /* Number of photons in the array that have
                        direction * normal > 0, where normal is
                        the supplied reconstruction normal. */

    protected Photon[] m_photons;
    protected float[] m_distances;
    protected float[] m_cosines; // photon dir * reconstruction normal
    protected boolean m_cosinesOk; // indicates if cosines are computed

    // nearest photon queries must use these functions!
    protected int
    doQuery(
        Vector3D position,
        int numberOfPhotons,
        float maximumRadius,
        short excludeFlags)
    {
        m_cosinesOk = false;
        return m_kdtree.query(
            new float[] {position.x, position.y, position.z},
            numberOfPhotons,
            m_photons,
            m_distances,
            maximumRadius,
            excludeFlags);
    }

    protected int doQuery(Vector3D pos) {
        m_cosinesOk = false;
        return m_kdtree.query(
            new float[] {pos.x, pos.y, pos.z},
            m_estimate_nrp[0] /*pmapstate.reconPhotons*/,
            m_photons,
            m_distances,
            (float)GetMaxR2(),
            (short)0);
    }

    protected IrrPhoton
    DoIrradianceQuery(Vector3D position, Vector3D normal, float maxR2) {
        return m_kdtree.normalPhotonQuery(position, normal, 0.8f, maxR2);
    }

    protected IrrPhoton
    DoIrradianceQuery(Vector3D position, Vector3D normal) {
        return DoIrradianceQuery(position, normal, Numeric.HUGE_FLOAT_VALUE);
    }

    // Compute cosines of photons with a supplied normal
    protected void computeCosines(Vector3D normal) {
        if ( !m_cosinesOk ) {
            m_nrpCosinePos = 0;

            for ( int i = 0; i < m_nrpFound; i++ ) {
                Vector3D dir = m_photons[i].dir();
                m_cosines[i] = dir.dotProduct(normal);
                if ( m_cosines[i] > 0 ) {
                    m_nrpCosinePos++;
                }
            }

            m_cosinesOk = true;
        }
    }

    // Add a photon taking possible irrPhoton into account
    protected void doAddPhoton(Photon photon, Vector3D normal, short flags) {
        if ( m_precomputeIrradiance ) {
            IrrPhoton irrPhoton = new IrrPhoton();
            irrPhoton.copy(photon);
            irrPhoton.setNormal(normal);
            m_kdtree.addPoint(irrPhoton, flags);
        } else {
            m_kdtree.addPoint(photon, flags);
        }
    }

    public static boolean
    zeroAlbedo(PhongBidirectionalScatteringDistributionFunction bsdf, RayHit hit, byte flags) {
        ColorRgb color;
        if ( bsdf == null ) {
            color = new ColorRgb();
            color.clear();
        } else {
            color = bsdf.splitBsdfScatteredPower(hit.shadingContext(), flags & 0xFF);
        }
        return (color.average() < Numeric.EPSILON);
    }

    private static float getFalseMonochrome(float val, PhotonMapState photonMapState) {
        float max = photonMapState.falseColMax;

        if ( photonMapState.falseColLog != 0 ) {
            max = (float)Math.log(1.0 + max);
            val = (float)Math.log(1.0 + val);
        }

        float tmp = Math.min(val, max);
        tmp = (tmp / max);

        return tmp;
    }

    // Convert a value val given a maximum into some nice color
    public static ColorRgb getFalseColor(float val, PhotonMapState photonMapState) {
        ColorRgb col = new ColorRgb();
        float tmp;
        float r = 0;
        float g = 0;
        float b = 0;

        if ( photonMapState.falseColMono != 0 ) {
            tmp = PhotonMap.getFalseMonochrome(val, photonMapState);
            col.set(tmp, tmp, tmp);
            return col;
        }

        float max = photonMapState.falseColMax;

        if ( photonMapState.falseColLog != 0 ) {
            max = (float)Math.log(1.0 + max);
            val = (float)Math.log(1.0 + val);
        }

        tmp = Math.min(val, max);

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

    public PhotonMap(PhotonMapState inPhotonMapState, int[] estimate_nrp, boolean doPrecomputeIrradiance) {
        photonMapState = inPhotonMapState;
        m_sample_nrp = 0;
        m_nrpCosinePos = 0;

        m_balanced = true;
        m_doBalancing = false;

        m_estimate_nrp = estimate_nrp != null ? estimate_nrp : new int[] {0};

        m_precomputeIrradiance = doPrecomputeIrradiance;
        m_irradianceComputed = false;

        if ( doPrecomputeIrradiance ) {
            m_kdtree = new PhotonKDTree(0, true);
        } else {
            m_kdtree = new PhotonKDTree(0, true);
        }

        m_totalPaths = 0;
        m_nrPhotons = 0;
        m_totalPhotons = 0;

        m_grid = new SampleGrid2D(2, 4);
        m_sampleLastPos = new Vector3D(Numeric.HUGE_FLOAT_VALUE, Numeric.HUGE_FLOAT_VALUE, Numeric.HUGE_FLOAT_VALUE);

        m_photons = new Photon[PhotonMapState.MAXIMUM_RECON_PHOTONS];
        m_distances = new float[PhotonMapState.MAXIMUM_RECON_PHOTONS];
        m_cosines = new float[PhotonMapState.MAXIMUM_RECON_PHOTONS];

        m_nrpFound = 0;  // No valid photons in array
        m_cosinesOk = true;
    }

    public PhotonMap(PhotonMapState inPhotonMapState, int[] estimate_nrp) {
        this(inPhotonMapState, estimate_nrp, false);
    }

    public void dispose() {
        m_photons = null;
        m_distances = null;
        m_cosines = null;
        m_grid = null;
        m_kdtree = null;
    }

    public void setTotalPaths(long totalPaths) {
        m_totalPaths = totalPaths;
    }

    public void printStats(PrintStream stream) {
        if ( stream == null ) {
            return;
        }
        stream.printf("%d stored photons\n", m_nrPhotons);
    }

    public void getStats(StringBuilder p, int n) {
        if ( p == null || n <= 0 ) {
            return;
        }

        String text = String.format("%d stored photons, %d total, %d paths\n", m_nrPhotons, m_totalPhotons, m_totalPaths);
        int available = Math.max(0, n - p.length());
        if ( available <= 0 ) {
            return;
        }
        if ( text.length() > available ) {
            p.append(text, 0, available);
        } else {
            p.append(text);
        }
    }

    /**
Adding photons, returns if photon was added
*/
    public boolean addPhoton(Photon photon, Vector3D normal, short flags) {
        Math.random(); // Just to keep in sync with density controlled storage

        doAddPhoton(photon, normal, flags);
        m_nrPhotons++;
        m_totalPhotons++;
        m_balanced = false;
        m_irradianceComputed = false;

        return true;
    }

    private static double
    computeAcceptProb(
        float currentD,
        float requiredD,
        PhotonMapState photonMapState)
    {
        // Step function
        if ( photonMapState.acceptPdfType == PhotonMapDCAcceptPDFType.STEP ) {
            if ( currentD > requiredD ) {
                return 0.0;
            } else {
                return 1.0;
            }
        } else if ( photonMapState.acceptPdfType == PhotonMapDCAcceptPDFType.TRANS_COSINE ) {
            // Translated cosine
            double ratio = Math.min(1.0, currentD / requiredD); // in [0,1]

            return (0.5 * (1.0 + Math.cos(ratio * Math.PI)));
        } else {
            Logger.error("PhotonMap::computeAcceptProb", "Unknown accept pdf type");
            return 0.0;
        }
    }

    public void redistribute(Photon photon) {
        // redistribute this photon over the nearest neighbours
        // m_distances, m_photons and m_cosines should be filled correctly!
        // only photons are used for which direction * normal > 0

        // -- Check the flags
        // -- normal weighted average?

        ColorRgb deltaPower = new ColorRgb();
        float factor = 1.0f / (float)m_nrpCosinePos;

        ColorRgb pow = photon.power();
        deltaPower.scaledCopy(factor, pow);

        for ( int i = 0; i < m_nrpFound; i++ ) {
            if ( m_cosines[i] > 0.0 ) {
                m_photons[i].addPower(deltaPower);
            }
        }
    }

    public boolean
    DC_AddPhoton(
        Photon photon,
        RayHit hit,
        float requiredD,
        short flags)
    {
        // Get current density
        // Vector3D pos = photon.Pos();
        boolean stored;

        float currentD = getCurrentDensity(hit, photonMapState.distribPhotons);
        // m_photons and m_distances is valid now !!

        // Compute acceptance probability

        double acceptProb = PhotonMap.computeAcceptProb(currentD, requiredD, photonMapState);

        // Debug trace for acceptance probability and density values.

        // Roulette
        if ( Math.random() < acceptProb ) {
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
    public double GetMaxR2() {
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
        final double radFraction = 0.03;

        if ( m_totalPaths <= 0 ) {
            return Numeric.HUGE_DOUBLE_VALUE;
        }

        return ((double)m_estimate_nrp[0] * Statistics.instance().radiance.totalArea /
                    (Math.PI * (double)m_totalPaths * radFraction));
    }

    // Precompute Irradiance
    public void photonPrecomputeIrradiance(Camera camera, IrrPhoton photon) {
        ColorRgb irradiance = new ColorRgb();
        irradiance.clear();

        // Locate the nearest photons using a max radius limit
        Vector3D pos = photon.pos();
        m_nrpFound = doQuery(pos);

        if ( m_nrpFound > 3 ) {
            // Construct irradiance estimate
            float maxDistance = m_distances[0];

            for ( int i = 0; i < m_nrpFound; i++ ) {
                if ( photon.Normal().dotProduct(m_photons[i].dir()) > 0 ) {
                    ColorRgb power = m_photons[i].power();
                    irradiance.add(irradiance, power);
                }
            }

            // Now we have incoming radiance integrated over area estimate,
            // so we convert it to irradiance, maxDistance is already squared
            // An extra factor PI is added, that accounts for Albedo -> diffuse brdf...
            float factor = (1.0f / ((float)Math.PI * (float)Math.PI * maxDistance * (float)m_totalPaths));
            irradiance.scale(factor);
        }

        photon.SetIrradiance(irradiance);
    }

    // Precompute irradiance
    public void precomputeIrradiance() {
        System.err.printf("PhotonMap::precomputeIrradiance\n");
        if ( m_precomputeIrradiance && !m_irradianceComputed ) {
            m_kdtree.iterateNodes((data, nodeData) -> {
                PhotonMap map = (PhotonMap)data;
                IrrPhoton photon = (IrrPhoton)nodeData;
                map.photonPrecomputeIrradiance(null, photon);
            }, this);
            m_irradianceComputed = true;
        }
    }

    public boolean
    irradianceReconstruct(
        RayHit hit,
        Vector3D outDir,
        ColorRgb diffuseAlbedo,
        ColorRgb result)
    {
        if ( !m_irradianceComputed ) {
            precomputeIrradiance();
        }

        Vector3D normal = hit.getNormal();
        Vector3D position = hit.getPoint();
        IrrPhoton photon = DoIrradianceQuery(position, normal);
        hit.setNormal(normal);

        if ( photon != null ) {
            result.scalarProduct(photon.m_irradiance, diffuseAlbedo);
            return true;
        } else {
            // No appropriate photon found
            return false;
        }
    }

    // reconstruct
    public ColorRgb reconstruct(
        RayHit hit,
        Vector3D outDir,
        PhongBidirectionalScatteringDistributionFunction bsdf,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf)
    {
        // Find the nearest photons
        ColorRgb result = new ColorRgb();
        ColorRgb eval = new ColorRgb();
        ColorRgb col = new ColorRgb();

        result.clear();

        ColorRgb diffuseAlbedo = new ColorRgb();
        ColorRgb glossyAlbedo = new ColorRgb();

        diffuseAlbedo.clear();
        glossyAlbedo.clear();

        if ( bsdf != null ) {
            diffuseAlbedo = bsdf.splitBsdfScatteredPower(hit.shadingContext(), BsdfComponent.BRDF_DIFFUSE_COMPONENT);
            // -- TODO Irradiance pre-computation for diffuse transmission
            glossyAlbedo = bsdf.splitBsdfScatteredPower(hit.shadingContext(),
                BsdfComponent.BTDF_DIFFUSE_COMPONENT | BsdfComponent.BRDF_GLOSSY_COMPONENT | BsdfComponent.BTDF_GLOSSY_COMPONENT);
        }

        checkNBalance();

        if ( glossyAlbedo.average() < Numeric.EPSILON ) {
            if ( diffuseAlbedo.average() < Numeric.EPSILON ) {
                return result; // No reflectance
            } else {
                if ( m_precomputeIrradiance ) {
                    if ( irradianceReconstruct(hit, outDir, diffuseAlbedo, result) ) {
                        return result;
                    } else {
                        // No appropriate irradiance photon -> do normal reconstruction
                    }
                }
            }
        }

        // Normal reconstruct...

        // Locate nearest photons using a max radius limit
        Vector3D position = hit.getPoint();
        m_nrpFound = doQuery(position);

        if ( m_nrpFound < 3 ) {
            return result;
        }

        // Construct radiance estimate
        float maxDistance = m_distances[0];

        for ( int i = 0; i < m_nrpFound; i++ ) {
            Vector3D dir = m_photons[i].dir();

            if ( bsdf == null ) {
                eval.clear();
            } else {
                eval = bsdf.evaluate(
                    hit.shadingContext(),
                    inBsdf,
                    outBsdf,
                    outDir,
                    dir,
                    BsdfComponent.BRDF_DIFFUSE_COMPONENT |
                    BsdfComponent.BTDF_DIFFUSE_COMPONENT |
                    BsdfComponent.BRDF_GLOSSY_COMPONENT |
                    BsdfComponent.BTDF_GLOSSY_COMPONENT);
            }
            ColorRgb power = m_photons[i].power();

            col.scalarProduct(eval, power);
            result.add(result, col);
        }

        // Now we have a radiance integrated over area estimate,
        // so we convert it to radiance, maxDistance is already squared

        float factor = 1.0f / ((float)Math.PI * maxDistance * (float)m_totalPaths);

        result.scale(factor);

        return result;
    }

    public float getCurrentDensity(RayHit hit, int nrPhotons) {
        // Find the nearest photons
        if ( nrPhotons == 0 ) {
            nrPhotons = m_estimate_nrp[0];
        }

        if ( nrPhotons == 0 ) {
            return 0.0f;
        }

        Vector3D position = hit.getPoint();
        m_nrpFound = doQuery(position, nrPhotons, (float)GetMaxR2(), (short)0);

        if ( m_nrpFound < 3 ) {
            return 0.0f;
        }

        // Construct density estimate
        float maxDistance = m_distances[0]; // Only valid since max heap is used in kdtree

        computeCosines(hit.getGeometricNormal()); // Shading normal?

        if ( m_nrpCosinePos <= 3 ) {
            return 0.0f;
        }

        return (float)(m_nrpCosinePos / (Math.PI * maxDistance));
    }

    /**
Return a color coded density of the photon map
*/
    public ColorRgb getDensityColor(RayHit hit) {
        float density = getCurrentDensity(hit, 0);

        return PhotonMap.getFalseColor(density, photonMapState);
    }

    // Sample values: Random values r,s are transformed into new
    // random values so that importance sampling using the photon
    // map is incorporated

    // IN: r,s in [0,1[
    //     coord: coordinate system that determines angles
    //     flags: component that will be sampled (GR or DR !!)
    //     n: phong exponent for GR

    // OUT: r,s are changed for importance sampling, probabilityDensityFunction is returned
    public double
    sample(Vector3D position, double[] r, double[] s, CoordinateSystem coord, byte flag, float n) {
        // -- Epsilon in as a function of scene/camera measure ??
        if ( !m_sampleLastPos.equals(position, 0.0001f) ) {
            // Need a new grid

            m_grid.init();

            // Find the nearest photons
            m_nrpFound = doQuery(position, m_sample_nrp, vsdk.toolkit.common.dataStructures.KDTree.KD_MAX_RADIUS, PhotonFlags.NO_IMPSAMP_PHOTON);

            double[] pr = new double[1];
            double[] ps = new double[1];

            for ( int i = 0; i < m_nrpFound; i++ ) {
                // Section [ARVO1995b].2: each photon direction is re-parameterized
                // in a local spherical frame before building the 2D sampling grid.
                m_photons[i].findRS(pr, ps, coord, flag, n);

                ColorRgb color = m_photons[i].power();

                m_grid.add(pr[0], ps[0], color.average() / (float)m_nrPhotons);
            }

            m_grid.EnsureNonZeroEntries();
            m_sampleLastPos.copy(position); // Caching
        }

        // Sample
        double[] probabilityDensityFunction = new double[1];
        m_grid.sample(r, s, probabilityDensityFunction);

        return probabilityDensityFunction[0];
    }

    public void Balance() {
        m_kdtree.balance();
    }

    public void checkNBalance() {
        if ((!m_balanced) && (m_doBalancing || m_precomputeIrradiance) ) {
            Balance();
            m_balanced = true;
        }
    }

    public void doBalancing(boolean state) {
        m_doBalancing = state;
    }
}

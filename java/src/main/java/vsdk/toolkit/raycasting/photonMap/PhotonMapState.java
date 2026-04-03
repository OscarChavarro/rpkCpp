package vsdk.toolkit.raycasting.photonMap;

public class PhotonMapState {
    public static final int MAXIMUM_RECON_PHOTONS = 400;

    public int doGlobalMap;
    public long gPathsPerIteration;
    public int precomputeGIrradiance;
    public int doCausticMap;
    public long cPathsPerIteration;
    public int renderImage;
    public int reconGPhotons;
    public int reconCPhotons;
    public int reconIPhotons;
    public int distribPhotons;
    public int balanceKDTree;
    public int usePhotonMapSampler;
    public PhotonMapDensityControlOption densityControl;
    public int importanceOption;
    public PhotonMapDCAcceptPDFType acceptPdfType;
    public float constantRD;
    public float minimumImpRD;
    public int doImportanceMap;
    public long iPathsPerIteration;
    public float cImpScale;
    public float gImpScale;
    public float gThreshold;
    public float falseColMax;
    public int falseColLog;
    public int falseColMono;
    public RadiosityReturnOption radianceReturn;
    public int minimumLightPathDepth;
    public int maximumLightPathDepth;
    public int iterationNumber;
    public int gIterationNumber;
    public int cIterationNumber;
    public int i_iteration_nr;
    public long totalCPaths;
    public long totalGPaths;
    public long totalIPaths;
    public int runStopNumber; // Number of 'external iterations'. This is
                             // different from currentIteration only when
                             // statistics are gathered
    public float cpuSecs; // For counting computing times
    public long lastClock;

    public PhotonMapState() {
        // Set other defaults, that can be reset multiple times
        setDefaults();
    }

    public void setDefaults() {
        // This is the only place where default values may be given...
        doCausticMap = 1;
        cPathsPerIteration = 20000;

        doGlobalMap = 1;
        gPathsPerIteration = 10000;
        precomputeGIrradiance = 1;

        renderImage = 0;

        reconGPhotons = 80;
        reconCPhotons = 80;
        reconIPhotons = 200;
        distribPhotons = 20;

        balanceKDTree = 1;
        usePhotonMapSampler = 0;

        densityControl = PhotonMapDensityControlOption.NO_DENSITY_CONTROL;
        acceptPdfType = PhotonMapDCAcceptPDFType.STEP;

        constantRD = 10000;

        minimumImpRD = 1;
        doImportanceMap = 1;
        iPathsPerIteration = 10000;

        importanceOption = PhotonMapImportanceOptions.USE_IMPORTANCE;

        cImpScale = 25.0f;
        gImpScale = 1.0f;

        gThreshold = 1000;

        falseColMax = 10000;
        falseColLog = 0;
        falseColMono = 0;

        radianceReturn = RadiosityReturnOption.GLOBAL_RADIANCE;

        minimumLightPathDepth = 0;
        maximumLightPathDepth = 7;
    }
}

package vsdk.toolkit.common.statistics;

public class Statistics {
    public ReaderStatistics reader;
    public RadianceStatistics radiance;
    public PotentialStatistics potential;
    public ShadowStatistics shadow;
    public RayTracerStatistics rayTracer;

    private static Statistics instanceValue;

    public Statistics() {
        reader = new ReaderStatistics();
        radiance = new RadianceStatistics();
        potential = new PotentialStatistics();
        shadow = new ShadowStatistics();
        rayTracer = new RayTracerStatistics();
    }

    public static Statistics instance() {
        if (instanceValue == null) {
            instanceValue = new Statistics();
        }
        return instanceValue;
    }
}

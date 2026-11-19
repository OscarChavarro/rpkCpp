package vsdk.toolkit.common.statistics;

public class RayTracerStatistics {
    public double totalTime;
    public long rayCount;
    public long pixelCount;

    public RayTracerStatistics() {
        totalTime = 0.0;
        rayCount = 0;
        pixelCount = 0;
    }

    public void resetCounters() {
        totalTime = 0.0;
        rayCount = 0;
        pixelCount = 0;
    }
}

package vsdk.toolkit.common.statistics;

import vsdk.toolkit.common.color.ColorRgbMutable;

public class RadianceStatistics {
    public float totalArea;
    public ColorRgbMutable maxSelfEmittedRadiance;
    public ColorRgbMutable maxSelfEmittedPower;
    public double referenceLuminance;
    public ColorRgbMutable totalEmittedPower;
    public ColorRgbMutable estimatedAverageRadiance;
    public ColorRgbMutable averageReflectivity;

    public RadianceStatistics() {
        totalArea = 0.0f;
        maxSelfEmittedRadiance = new ColorRgbMutable();
        maxSelfEmittedPower = new ColorRgbMutable();
        referenceLuminance = 0.0;
        totalEmittedPower = new ColorRgbMutable();
        estimatedAverageRadiance = new ColorRgbMutable();
        averageReflectivity = new ColorRgbMutable();
    }

    public void reset() {
        totalArea = 0.0f;
        maxSelfEmittedRadiance = new ColorRgbMutable();
        maxSelfEmittedPower = new ColorRgbMutable();
        referenceLuminance = 0.0;
        totalEmittedPower = new ColorRgbMutable();
        estimatedAverageRadiance = new ColorRgbMutable();
        averageReflectivity = new ColorRgbMutable();
    }
}

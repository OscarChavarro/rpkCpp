package vsdk.toolkit.common.statistics;

import vsdk.toolkit.common.color.ColorRgb;

public class RadianceStatistics {
    public float totalArea;
    public ColorRgb maxSelfEmittedRadiance;
    public ColorRgb maxSelfEmittedPower;
    public double referenceLuminance;
    public ColorRgb totalEmittedPower;
    public ColorRgb estimatedAverageRadiance;
    public ColorRgb averageReflectivity;

    public RadianceStatistics() {
        totalArea = 0.0f;
        maxSelfEmittedRadiance = new ColorRgb();
        maxSelfEmittedPower = new ColorRgb();
        referenceLuminance = 0.0;
        totalEmittedPower = new ColorRgb();
        estimatedAverageRadiance = new ColorRgb();
        averageReflectivity = new ColorRgb();
    }

    public void reset() {
        totalArea = 0.0f;
        maxSelfEmittedRadiance.clear();
        maxSelfEmittedPower.clear();
        referenceLuminance = 0.0;
        totalEmittedPower.clear();
        estimatedAverageRadiance.clear();
        averageReflectivity.clear();
    }
}

/**
Encapsulates option handling for bidirectional path tracing
*/

package vsdk.toolkit.raycasting.bidirectionalRaytracing;

/**
Contains basic config options used throughout the complete BPT code. typically two
instances are used. One persistent, containing current GUI options and one non
persistent copy when rendering an image
*/
public class BidirectionalPathRaytracerConfig {
    public static final int MAX_REGEXP_SIZE = 100;

    // Path depth config
    public int maximumEyePathDepth;
    public int maximumLightPathDepth;
    public int maximumPathDepth;
    public int minimumPathDepth;

    // Sampling details
    public int samplesPerPixel;
    public long totalSamples;
    public int sampleImportantLights;
    public int progressiveTracing;
    public int eliminateSpikes;

    // SPaR details (usage of reg exps)
    public int useSpars;
    public int doLe;
    public int doLD;
    public int doLI;

    public String leRegExp = "";
    public String ldRegExp = "";
    public String liRegExp = "";

    public int doWeighted; // Use weighted multi-pass method for a part of le and ld transport
    public String wleRegExp = "";
    public String wldRegExp = "";

    // These values are not yet in the presets or UI!
    public int doDensityEstimation;

    public void copyFrom(BidirectionalPathRaytracerConfig other) {
        if ( other == null ) {
            return;
        }
        maximumEyePathDepth = other.maximumEyePathDepth;
        maximumLightPathDepth = other.maximumLightPathDepth;
        maximumPathDepth = other.maximumPathDepth;
        minimumPathDepth = other.minimumPathDepth;
        samplesPerPixel = other.samplesPerPixel;
        totalSamples = other.totalSamples;
        sampleImportantLights = other.sampleImportantLights;
        progressiveTracing = other.progressiveTracing;
        eliminateSpikes = other.eliminateSpikes;
        useSpars = other.useSpars;
        doLe = other.doLe;
        doLD = other.doLD;
        doLI = other.doLI;
        leRegExp = other.leRegExp;
        ldRegExp = other.ldRegExp;
        liRegExp = other.liRegExp;
        doWeighted = other.doWeighted;
        wleRegExp = other.wleRegExp;
        wldRegExp = other.wldRegExp;
        doDensityEstimation = other.doDensityEstimation;
    }
}

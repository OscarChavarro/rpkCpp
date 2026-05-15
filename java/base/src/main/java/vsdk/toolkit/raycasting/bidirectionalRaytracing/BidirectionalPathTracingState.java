package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.render.ScreenBuffer;

/**
Persistent BPT state
*/
public class BidirectionalPathTracingState {
    public BidirectionalPathRaytracerConfig baseConfig;
    public ScreenBuffer lastScreen;
    public String baseFilename;
    public int saveSubsequentImages;

    public BidirectionalPathTracingState() {
        baseConfig = new BidirectionalPathRaytracerConfig();
        lastScreen = null;
        saveSubsequentImages = 0;

        baseConfig.samplesPerPixel = 1;
        baseConfig.progressiveTracing = 1;
        baseConfig.minimumPathDepth = 2;
        baseConfig.maximumPathDepth = 7;
        baseConfig.maximumEyePathDepth = 7;
        baseConfig.maximumLightPathDepth = 7;
        baseConfig.sampleImportantLights = 1;
        baseConfig.useSpars = 0;
        baseConfig.doLe = 1;
        baseConfig.doLD = 0;
        baseConfig.doLI = 0;

        baseConfig.doWeighted = 0;

        baseConfig.leRegExp = "(LX)(X)*(EX)";
        baseConfig.ldRegExp = "(LX)(G|S)(X)*(EX),(LX)(EX)";
        baseConfig.liRegExp = "(LX)(G|S)(X)*(EX),(LX)(EX)";
        baseConfig.wleRegExp = "(LX)(DR)(X)*(EX)";
        baseConfig.wldRegExp = "(LX)(X)*(EX)";

        baseConfig.eliminateSpikes = 0;
        baseConfig.doDensityEstimation = 0;
        baseFilename = "";
    }
}

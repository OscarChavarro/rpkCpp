/**
Options and runtime state for stochastic raytracing.
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.render.ScreenBuffer;

public class StochasticRayTracingState {
    public StochasticRayTracingState() {
        samplesPerPixel = 1;
        progressiveTracing = 1;
        doFrameCoherent = 0;
        doCorrelatedSampling = 0;
        baseSeed = 0xFE062134L;
        radMode = RayTracingRadMode.STORED_NONE;
        nextEvent = 1;
        nextEventSamples = 1;
        lightMode = RayTracingLightMode.ALL_LIGHTS;
        backgroundDirect = 0;
        backgroundIndirect = 1;
        backgroundSampling = 0;
        scatterSamples = 1;
        differentFirstDG = 0;
        firstDGSamples = 36;
        separateSpecular = 0;
        reflectionSampling = RayTracingSamplingMode.BRDF_SAMPLING;
        minPathDepth = 5;
        maxPathDepth = 7;
        lastScreen = null;
    }

    // Pixel sampling
    public int samplesPerPixel;
    public int progressiveTracing;

    public int doFrameCoherent;
    public int doCorrelatedSampling;
    public long baseSeed;

    // Stored radiance handling
    public RayTracingRadMode radMode;

    // Direct Light sampling
    public int nextEvent;
    public int nextEventSamples;
    public RayTracingLightMode lightMode;

    // Background
    public int backgroundDirect;
    public int backgroundIndirect;
    public int backgroundSampling;

    // Scattering
    public int scatterSamples;
    public int differentFirstDG;
    public int firstDGSamples;
    public int separateSpecular;
    public RayTracingSamplingMode reflectionSampling;

    public int minPathDepth;
    public int maxPathDepth;

    public ScreenBuffer lastScreen;
}

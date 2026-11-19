package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.raycasting.raytracing.SamplerConfig;
import vsdk.toolkit.render.ScreenBuffer;
import vsdk.toolkit.tonemap.ToneMappingContext;

/**
Bidirectional path tracing configuration structure.
non persistently used each time an image is rendered
*/
public class BidirectionalPathTracingConfiguration {
    public BidirectionalPathRaytracerConfig baseConfig;

    // Configuration for tracing the paths
    public SamplerConfig eyeConfig;
    public SamplerConfig lightConfig;

    // Internal vars
    public ScreenBuffer screen;
    public ToneMappingContext toneMapOptions;
    public double fluxToRadFactor;
    public int nx;
    public int ny;
    public double pdfLNE; // pdf for sampling light point separately

    public DensityBuffer dBuffer;
    public DensityBuffer dBuffer2;
    public float xSample;
    public float ySample;
    public SimpleRaytracingPathNode eyePath;
    public SimpleRaytracingPathNode lightPath;

    // SPaR configuration
    public SparConfig sparConfig;
    public SparList sparList;
    public boolean deStoreHits;
    public ScreenBuffer ref;
    public ScreenBuffer dest;
    public ScreenBuffer ref2;
    public ScreenBuffer dest2;
    public Kernel2D kernel;
    public int scaleSamples;

    public BidirectionalPathTracingConfiguration() {
        eyeConfig = new SamplerConfig();
        lightConfig = new SamplerConfig();
        sparConfig = new SparConfig();
        kernel = new Kernel2D();
    }
}

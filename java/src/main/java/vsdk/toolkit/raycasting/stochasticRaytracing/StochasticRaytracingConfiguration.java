package vsdk.toolkit.raycasting.stochasticRaytracing;

import java.util.ArrayList;

import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.ImportantLightSampler;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.LightList;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.UniformLightSampler;
import vsdk.toolkit.raycasting.photonMap.PhotonMapSampler;
import vsdk.toolkit.raycasting.raytracing.BsdfSampler;
import vsdk.toolkit.raycasting.raytracing.EyeSampler;
import vsdk.toolkit.raycasting.raytracing.PixelSampler;
import vsdk.toolkit.raycasting.raytracing.SamplerConfig;
import vsdk.toolkit.render.ScreenBuffer;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.RadianceMethodAlgorithm;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.tonemap.ToneMappingContext;

public class StochasticRaytracingConfiguration {
    public int samplesPerPixel;

    public int nextEventSamples;
    public RayTracingLightMode lightMode;

    public RayTracingRadMode radMode;

    public int scatterSamples;
    public int firstDGSamples;
    public RayTracingSamplingMode reflectionSampling;
    public boolean separateSpecular;

    public boolean backgroundIndirect; // Use background in reflections (indirect)
    public boolean backgroundDirect; // Use background when no surface is hit (direct)
    public boolean backgroundSampling; // Use light sampling
    public int doFrameCoherent;
    public int doCorrelatedSampling;
    public long baseSeed;

    // Independent variables
    public ScreenBuffer screen;
    public ToneMappingContext toneMapOptions;

    // Variables derived from user options
    // All variables must not change during raytracing...
    public SamplerConfig samplerConfig;
    public SeedConfig seedConfig;

    // Scatter info blocks

    // Scattering info for the part of light
    // transport that is used from storage
    // (Diffuse for getTopLevelPatchRad, Diffuse & Glossy for photon map methods)
    public ScatterInfo siStorage;

    // Maximum 1 scattering block per component
    public ScatterInfo[] siOthers;
    public int siOthersCount;

    public StorageReadout initialReadout;
    public LightList rayTracingLightList;

    public StochasticRaytracingConfiguration(
        Camera defaultCamera,
        StochasticRayTracingState state,
        ArrayList<Patch> lightList,
        RadianceMethod radianceMethod,
        ToneMappingContext inToneMapOptions,
        LightList inRayTracingLightList)
    {
        samplesPerPixel = 0;
        nextEventSamples = 0;
        lightMode = RayTracingLightMode.ALL_LIGHTS;
        radMode = RayTracingRadMode.STORED_NONE;
        scatterSamples = 0;
        firstDGSamples = 0;
        reflectionSampling = RayTracingSamplingMode.BRDF_SAMPLING;
        separateSpecular = false;
        backgroundIndirect = false;
        backgroundDirect = false;
        backgroundSampling = false;
        doFrameCoherent = 0;
        doCorrelatedSampling = 0;
        baseSeed = 0L;
        screen = null;
        toneMapOptions = null;
        samplerConfig = new SamplerConfig();
        seedConfig = new SeedConfig();
        siStorage = new ScatterInfo();
        siOthers = new ScatterInfo[6];
        for ( int i = 0; i < siOthers.length; i++ ) {
            siOthers[i] = new ScatterInfo();
        }
        siOthersCount = 0;
        initialReadout = StorageReadout.SCATTER;
        rayTracingLightList = inRayTracingLightList;
        init(defaultCamera, state, lightList, radianceMethod, inToneMapOptions, inRayTracingLightList);
    }

    public void release() {
        samplerConfig.releaseVars();
    }

    public void init(
        Camera defaultCamera,
        StochasticRayTracingState state,
        ArrayList<Patch> lightList,
        RadianceMethod radianceMethod,
        ToneMappingContext inToneMapOptions,
        LightList inRayTracingLightList)
    {
        // Copy state options

        samplesPerPixel = state.samplesPerPixel;

        radMode = state.radMode;

        backgroundIndirect = state.backgroundIndirect != 0;
        backgroundDirect = state.backgroundDirect != 0;
        backgroundSampling = state.backgroundSampling != 0;
        doFrameCoherent = state.doFrameCoherent;
        doCorrelatedSampling = state.doCorrelatedSampling;
        baseSeed = state.baseSeed;

        if ( radMode != RayTracingRadMode.STORED_NONE ) {
            if ( radianceMethod == null ) {
                Logger.error("Stored Radiance", "No radiance method active, using no storage");
            } else if ( (radMode == RayTracingRadMode.STORED_PHOTON_MAP) && (radianceMethod.className != RadianceMethodAlgorithm.PHOTON_MAP) ) {
                Logger.error("Stored Radiance", "Photon map method not active, using no storage");
            }
            radMode = RayTracingRadMode.STORED_NONE;
        }

        if ( state.nextEvent != 0 ) {
            nextEventSamples = state.nextEventSamples;
        } else {
            nextEventSamples = 0;
        }
        lightMode = state.lightMode;

        reflectionSampling = state.reflectionSampling;

        if ( reflectionSampling == RayTracingSamplingMode.CLASSICAL_SAMPLING
          && radMode == RayTracingRadMode.STORED_INDIRECT ) {
            Logger.error("Classical raytracing", "Incompatible with extended final gather, using storage directly");
            radMode = RayTracingRadMode.STORED_DIRECT;
        }

        scatterSamples = state.scatterSamples;
        if ( state.differentFirstDG != 0 ) {
            firstDGSamples = state.firstDGSamples;
        } else {
            firstDGSamples = scatterSamples;
        }

        separateSpecular = state.separateSpecular != 0;

        if ( reflectionSampling == RayTracingSamplingMode.PHOTON_MAP_SAMPLING ) {
            Logger.warning("Fresnel Specular Sampling", "always uses separate specular");
            separateSpecular = true;  // Always separate specular with photon map
        }

        samplerConfig.minDepth = state.minPathDepth;
        samplerConfig.maxDepth = state.maxPathDepth;

        toneMapOptions = inToneMapOptions;
        if ( toneMapOptions == null ) {
            Logger.fatal(-1, "StochasticRaytracingConfiguration::init", "Tone mapping context not set");
        }

        screen = new ScreenBuffer(null, defaultCamera, toneMapOptions);
        screen.setFactor(1.0f); // We're storing plain radiance

        initDependentVars(lightList, radianceMethod, inRayTracingLightList);
    }

    private void initDependentVars(
        ArrayList<Patch> lightList,
        RadianceMethod radianceMethod,
        LightList inRayTracingLightList)
    {
        // Sampler configuration
        samplerConfig.pointSampler = new EyeSampler();
        samplerConfig.dirSampler = new PixelSampler();

        switch ( reflectionSampling ) {
            case BRDF_SAMPLING:
                samplerConfig.surfaceSampler = new BsdfSampler();
                break;
            case PHOTON_MAP_SAMPLING:
                samplerConfig.surfaceSampler = new PhotonMapSampler();
                break;
            case CLASSICAL_SAMPLING:
                // Java port currently maps classical sampling to BSDF sampler.
                samplerConfig.surfaceSampler = new BsdfSampler();
                break;
            default:
                Logger.error("SR CONFIG::initDependentVars", "Wrong sampling mode");
        }

        // Scatter info blocks
        // Storage block
        int storeFlags;

        int bsdfGlossy = BsdfComponent.BRDF_GLOSSY_COMPONENT | BsdfComponent.BTDF_GLOSSY_COMPONENT;
        int bsdfDiffuse = BsdfComponent.BRDF_DIFFUSE_COMPONENT | BsdfComponent.BTDF_DIFFUSE_COMPONENT;
        int bsdfAll = BsdfComponent.BRDF_DIFFUSE_COMPONENT
            | BsdfComponent.BRDF_GLOSSY_COMPONENT
            | BsdfComponent.BRDF_SPECULAR_COMPONENT
            | BsdfComponent.BTDF_DIFFUSE_COMPONENT
            | BsdfComponent.BTDF_GLOSSY_COMPONENT
            | BsdfComponent.BTDF_SPECULAR_COMPONENT;

        if ((radianceMethod == null) || (radMode == RayTracingRadMode.STORED_NONE) ) {
            storeFlags = 0;
        } else {
            if ( radianceMethod.className == RadianceMethodAlgorithm.PHOTON_MAP ) {
                storeFlags = bsdfGlossy | bsdfDiffuse;
            } else {
                storeFlags = BsdfComponent.BRDF_DIFFUSE_COMPONENT;
            }
        }

        initialReadout = StorageReadout.SCATTER;

        switch ( radMode ) {
            case STORED_NONE:
                siStorage.flags = (byte)0;
                siStorage.nrSamplesBefore = 0;
                siStorage.nrSamplesAfter = 0;
                break;
            case STORED_DIRECT:
                siStorage.flags = (byte)storeFlags;
                siStorage.nrSamplesBefore = 0;
                siStorage.nrSamplesAfter = 0;
                initialReadout = StorageReadout.READ_NOW;
                break;
            case STORED_INDIRECT:
            case STORED_PHOTON_MAP:
                siStorage.flags = (byte)storeFlags;
                siStorage.nrSamplesBefore = firstDGSamples;
                siStorage.nrSamplesAfter = 0;
                break;
            default:
                Logger.error("SR CONFIG::initDependentVars", "Wrong Rad Mode");
        }

        // Other blocks, this is non storage with optional
        // separation of specular components

        int remainingFlags = bsdfAll & ~storeFlags;
        int siIndex = 0;

        if ( separateSpecular ) {
            int flags;

            // spec reflection

            if ( reflectionSampling == RayTracingSamplingMode.CLASSICAL_SAMPLING ) {
                flags = remainingFlags & (BsdfComponent.BRDF_SPECULAR_COMPONENT |
                                          BsdfComponent.BRDF_GLOSSY_COMPONENT); // Glossy == Specular in classic
            } else {
                flags = remainingFlags & BsdfComponent.BRDF_SPECULAR_COMPONENT;
            }

            if ( flags != 0 ) {
                siOthers[siIndex].flags = (byte)flags;
                siOthers[siIndex].nrSamplesBefore = scatterSamples;
                siOthers[siIndex].nrSamplesAfter = scatterSamples;
                siIndex++;
                remainingFlags = remainingFlags & ~flags;
            }

            // Spec transmission
            if ( reflectionSampling == RayTracingSamplingMode.CLASSICAL_SAMPLING ) {
                flags = remainingFlags & (BsdfComponent.BTDF_SPECULAR_COMPONENT |
                                          BsdfComponent.BTDF_GLOSSY_COMPONENT); // Glossy == Specular in classic
            } else {
                flags = remainingFlags & BsdfComponent.BTDF_SPECULAR_COMPONENT;
            }

            if ( flags != 0 ) {
                siOthers[siIndex].flags = (byte)flags;
                siOthers[siIndex].nrSamplesBefore = scatterSamples;
                siOthers[siIndex].nrSamplesAfter = scatterSamples;
                siIndex++;
                remainingFlags = remainingFlags & ~flags;
            }
        }

        // Glossy or diffuse with different firstDGSamples

        if ( reflectionSampling != RayTracingSamplingMode.CLASSICAL_SAMPLING
           && scatterSamples != firstDGSamples ) {
            int gdFlags = remainingFlags &
                (bsdfDiffuse | bsdfGlossy);
            if ( gdFlags != 0 ) {
                siOthers[siIndex].flags = (byte)gdFlags;
                siOthers[siIndex].nrSamplesBefore = firstDGSamples;
                siOthers[siIndex].nrSamplesAfter = scatterSamples;
                siIndex++;
                remainingFlags = remainingFlags & ~gdFlags;
            }
        }

        if ( reflectionSampling == RayTracingSamplingMode.CLASSICAL_SAMPLING ) {
            // Classical: Diffuse, with no scattering
            int dFlags = remainingFlags & bsdfDiffuse;

            if ( dFlags != 0 ) {
                siOthers[siIndex].flags = (byte)dFlags;
                siOthers[siIndex].nrSamplesBefore = 0;
                siOthers[siIndex].nrSamplesAfter = 0;
                siIndex++;
                remainingFlags = remainingFlags & ~dFlags;
            }
        }

        // All other flags (possibly none) just get scattered normally
        if ( remainingFlags != 0 ) {
            siOthers[siIndex].flags = (byte)remainingFlags;
            siOthers[siIndex].nrSamplesBefore = scatterSamples;
            siOthers[siIndex].nrSamplesAfter = scatterSamples;
            siIndex++;
        }

        siOthersCount = siIndex;

        // Main init the light list
        rayTracingLightList = new LightList(lightList, backgroundSampling);

        if ( lightMode == RayTracingLightMode.IMPORTANT_LIGHTS ) {
            samplerConfig.neSampler = new ImportantLightSampler(rayTracingLightList);
        } else {
            samplerConfig.neSampler = new UniformLightSampler(rayTracingLightList);
        }

        // Main init the seed config
        seedConfig.init(samplerConfig.maxDepth);
    }
}

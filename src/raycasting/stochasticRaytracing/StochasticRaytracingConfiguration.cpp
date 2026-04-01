#include "common/Error.h"
#include "common/RenderOptions.h"
#include "scene/RadianceMethod.h"
#include "raycasting/raytracing/EyeSampler.h"
#include "raycasting/raytracing/SpecularSampler.h"
#include "raycasting/bidirectionalRaytracing/LightSampler.h"
#include "photonMap/PhotonMapSampler.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingConfiguration.h"

#ifdef RAYTRACING_ENABLED

CSeed CSeedConfig::xOrSeed;

void
StochasticRaytracingConfiguration::init(
    const Camera *defaultCamera,
    const StochasticRayTracingState &state,
    const java::ArrayList<Patch *> *lightList,
    const RadianceMethod *radianceMethod,
    ToneMappingContext *inToneMapOptions,
    LightList *&rayTracingLightList)
{
    // Copy state options

    samplesPerPixel = state.samplesPerPixel;

    radMode = state.radMode;

    backgroundIndirect = state.backgroundIndirect;
    backgroundDirect = state.backgroundDirect;
    backgroundSampling = state.backgroundSampling;
    doFrameCoherent = state.doFrameCoherent;
    doCorrelatedSampling = state.doCorrelatedSampling;
    baseSeed = state.baseSeed;

    if ( radMode != RayTracingRadMode::STORED_NONE ) {
        if ( radianceMethod == nullptr ) {
            Error::error("Stored Radiance", "No radiance method active, using no storage");
        } else if ( (radMode == RayTracingRadMode::STORED_PHOTON_MAP) && (radianceMethod->className != PHOTON_MAP) ) {
            Error::error("Stored Radiance", "Photon map method not active, using no storage");
        }
        radMode = RayTracingRadMode::STORED_NONE;
    }

    if ( state.nextEvent ) {
        nextEventSamples = state.nextEventSamples;
    } else {
        nextEventSamples = 0;
    }
    lightMode = state.lightMode;

    reflectionSampling = state.reflectionSampling;

    if ( reflectionSampling == RayTracingSamplingMode::CLASSICAL_SAMPLING
      && radMode == RayTracingRadMode::STORED_INDIRECT ) {
        Error::error("Classical raytracing", "Incompatible with extended final gather, using storage directly");
        radMode = RayTracingRadMode::STORED_DIRECT;
    }

    scatterSamples = state.scatterSamples;
    if ( state.differentFirstDG ) {
        firstDGSamples = state.firstDGSamples;
    } else {
        firstDGSamples = scatterSamples;
    }

    separateSpecular = state.separateSpecular;

    if ( reflectionSampling == RayTracingSamplingMode::PHOTON_MAP_SAMPLING ) {
        Error::warning("Fresnel Specular Sampling", "always uses separate specular");
        separateSpecular = true;  // Always separate specular with photon map
    }

    samplerConfig.minDepth = state.minPathDepth;
    samplerConfig.maxDepth = state.maxPathDepth;

    toneMapOptions = inToneMapOptions;
    if ( toneMapOptions == nullptr ) {
        Error::fatal(-1, "StochasticRaytracingConfiguration::init", "Tone mapping context not set");
    }

    screen = new ScreenBuffer(nullptr, defaultCamera, toneMapOptions);
    screen->setFactor(1.0); // We're storing plain radiance

    initDependentVars(lightList, radianceMethod, rayTracingLightList);
}

void
StochasticRaytracingConfiguration::initDependentVars(
    const java::ArrayList<Patch *> *lightList,
    const RadianceMethod *radianceMethod,
    LightList *&rayTracingLightList)
{
    // Sampler configuration
    samplerConfig.pointSampler = new EyeSampler;
    samplerConfig.dirSampler = new PixelSampler;

    switch ( reflectionSampling ) {
        case RayTracingSamplingMode::BRDF_SAMPLING:
            samplerConfig.surfaceSampler = new BsdfSampler;
            break;
        case RayTracingSamplingMode::PHOTON_MAP_SAMPLING:
            samplerConfig.surfaceSampler = new PhotonMapSampler;
            break;
        case RayTracingSamplingMode::CLASSICAL_SAMPLING:
            samplerConfig.surfaceSampler = new SpecularSampler;
            break;
        default:
            Error::error("SR CONFIG::initDependentVars", "Wrong sampling mode");
    }

    // Scatter info blocks
    // Storage block
    char storeFlags;

    if ((radianceMethod == nullptr) || (radMode == RayTracingRadMode::STORED_NONE) ) {
        storeFlags = NO_COMPONENTS;
    } else {
        if ( radianceMethod->className == PHOTON_MAP ) {
            storeFlags = BSDF_GLOSSY_COMPONENT | BSDF_DIFFUSE_COMPONENT;
        } else {
            storeFlags = BRDF_DIFFUSE_COMPONENT;
        }
    }

    initialReadout = StorageReadout::SCATTER;

    switch ( radMode ) {
        case RayTracingRadMode::STORED_NONE:
            siStorage.flags = NO_COMPONENTS;
            siStorage.nrSamplesBefore = 0;
            siStorage.nrSamplesAfter = 0;
            break;
        case RayTracingRadMode::STORED_DIRECT:
            siStorage.flags = storeFlags;
            siStorage.nrSamplesBefore = 0;
            siStorage.nrSamplesAfter = 0;
            initialReadout = StorageReadout::READ_NOW;
            break;
        case RayTracingRadMode::STORED_INDIRECT:
        case RayTracingRadMode::STORED_PHOTON_MAP:
            siStorage.flags = storeFlags;
            siStorage.nrSamplesBefore = firstDGSamples;
            siStorage.nrSamplesAfter = 0;
            break;
        default:
            Error::error("SR CONFIG::initDependentVars", "Wrong Rad Mode");
    }

    // Other blocks, this is non storage with optional
    // separation of specular components

    char remainingFlags = BSDF_ALL_COMPONENTS & ~storeFlags;
    int siIndex = 0;

    if ( separateSpecular ) {
        char flags;

        // spec reflection

        if ( reflectionSampling == RayTracingSamplingMode::CLASSICAL_SAMPLING ) {
            flags = static_cast<char>(remainingFlags & (BRDF_SPECULAR_COMPONENT |
                                                        BRDF_GLOSSY_COMPONENT)); // Glossy == Specular in classic
        } else {
            flags = static_cast<char>(remainingFlags & BRDF_SPECULAR_COMPONENT);
        }

        if ( flags ) {
            siOthers[siIndex].flags = flags;
            siOthers[siIndex].nrSamplesBefore = scatterSamples;
            siOthers[siIndex].nrSamplesAfter = scatterSamples;
            siIndex++;
            remainingFlags = static_cast<char>(remainingFlags & ~flags);
        }

        // Spec transmission
        if ( reflectionSampling == RayTracingSamplingMode::CLASSICAL_SAMPLING ) {
            flags = static_cast<char>(remainingFlags & (BTDF_SPECULAR_COMPONENT |
                                                        BTDF_GLOSSY_COMPONENT)); // Glossy == Specular in classic
        } else {
            flags = static_cast<char>(remainingFlags & BTDF_SPECULAR_COMPONENT);
        }

        if ( flags ) {
            siOthers[siIndex].flags = flags;
            siOthers[siIndex].nrSamplesBefore = scatterSamples;
            siOthers[siIndex].nrSamplesAfter = scatterSamples;
            siIndex++;
            remainingFlags = static_cast<char>(remainingFlags & ~flags);
        }
    }

    // Glossy or diffuse with different firstDGSamples

    if ( reflectionSampling != RayTracingSamplingMode::CLASSICAL_SAMPLING
       && scatterSamples != firstDGSamples ) {
        char gdFlags = static_cast<char>(remainingFlags &
                                         (BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT));
        if ( gdFlags ) {
            siOthers[siIndex].flags = gdFlags;
            siOthers[siIndex].nrSamplesBefore = firstDGSamples;
            siOthers[siIndex].nrSamplesAfter = scatterSamples;
            siIndex++;
            remainingFlags = static_cast<char>(remainingFlags & ~gdFlags);
        }
    }

    if ( reflectionSampling == RayTracingSamplingMode::CLASSICAL_SAMPLING ) {
        // Classical: Diffuse, with no scattering
        char dFlags = static_cast<char>(remainingFlags & BSDF_DIFFUSE_COMPONENT);

        if ( dFlags ) {
            siOthers[siIndex].flags = dFlags;
            siOthers[siIndex].nrSamplesBefore = 0;
            siOthers[siIndex].nrSamplesAfter = 0;
            siIndex++;
            remainingFlags = static_cast<char>(remainingFlags & ~dFlags);
        }
    }

    // All other flags (possibly none) just get scattered normally
    if ( remainingFlags ) {
        siOthers[siIndex].flags = remainingFlags;
        siOthers[siIndex].nrSamplesBefore = scatterSamples;
        siOthers[siIndex].nrSamplesAfter = scatterSamples;
        siIndex++;
    }

    siOthersCount = siIndex;

    // Main init the light list
    if ( rayTracingLightList != nullptr ) {
        delete rayTracingLightList;
    }

    rayTracingLightList = new LightList(lightList, backgroundSampling);

    if ( lightMode == RayTracingLightMode::IMPORTANT_LIGHTS ) {
        samplerConfig.neSampler = new ImportantLightSampler(rayTracingLightList);
    } else {
        samplerConfig.neSampler = new UniformLightSampler(rayTracingLightList);
    }

    // Main init the seed config
    seedConfig.init(samplerConfig.maxDepth);
}

#endif

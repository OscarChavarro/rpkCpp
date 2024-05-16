#include "common/error.h"
#include "scene/RadianceMethod.h"
#include "PHOTONMAP/photonmapsampler.h"
#include "raycasting/raytracing/eyesampler.h"
#include "raycasting/raytracing/bsdfsampler.h"
#include "raycasting/raytracing/specularsampler.h"
#include "raycasting/bidirectionalRaytracing/LightSampler.h"
#include "raycasting/stochasticRaytracing/rtstochasticphotonmap.h"

#ifdef RAYTRACING_ENABLED

CSeed CSeedConfig::xOrSeed;

void
StochasticRaytracingConfiguration::init(
    const Camera *defaultCamera,
    const StochasticRayTracingState &state,
    const java::ArrayList<Patch *> *lightList,
    const RadianceMethod *radianceMethod)
{
    // Copy state options

    samplesPerPixel = state.samplesPerPixel;

    radMode = state.radMode;

    backgroundIndirect = state.backgroundIndirect;
    backgroundDirect = state.backgroundDirect;
    backgroundSampling = state.backgroundSampling;

    if ( radMode != STORED_NONE ) {
        if ( radianceMethod == nullptr ) {
            logError("Stored Radiance", "No radiance method active, using no storage");
            radMode = STORED_NONE;
        } else if ( (radMode == STORED_PHOTON_MAP) && (radianceMethod->className != PHOTON_MAP) ) {
            logError("Stored Radiance", "Photon map method not active, using no storage");
            radMode = STORED_NONE;
        }
    }

    if ( state.nextEvent ) {
        nextEventSamples = state.nextEventSamples;
    } else {
        nextEventSamples = 0;
    }
    lightMode = state.lightMode;

    reflectionSampling = state.reflectionSampling;

    if ( reflectionSampling == CLASSICAL_SAMPLING
        && ( radMode == STORED_INDIRECT || radMode == STORED_PHOTON_MAP ) ) {
        logError("Classical raytracing",
                 "Incompatible with extended final gather, using storage directly");
        radMode = STORED_DIRECT;
    }

    if ( radMode == STORED_PHOTON_MAP ) {
        logWarning("Photon map reflection Sampling",
                   "Make sure to use the same sampling (brdf, fresnel) as for photon map construction");
    }

    scatterSamples = state.scatterSamples;
    if ( state.differentFirstDG ) {
        firstDGSamples = state.firstDGSamples;
    } else {
        firstDGSamples = scatterSamples;
    }

    separateSpecular = state.separateSpecular;

    if ( reflectionSampling == PHOTON_MAP_SAMPLING ) {
        logWarning("Fresnel Specular Sampling", "always uses separate specular");
        separateSpecular = true;  // Always separate specular with photon map
    }

    samplerConfig.minDepth = state.minPathDepth;
    samplerConfig.maxDepth = state.maxPathDepth;

    screen = new ScreenBuffer(nullptr, defaultCamera);
    screen->setFactor(1.0); // We're storing plain radiance

    initDependentVars(lightList, radianceMethod);
}

void
StochasticRaytracingConfiguration::initDependentVars(
    const java::ArrayList<Patch *> *lightList,
    const RadianceMethod *radianceMethod)
{
    // Sampler configuration
    samplerConfig.pointSampler = new CEyeSampler;
    samplerConfig.dirSampler = new CPixelSampler;

    switch ( reflectionSampling ) {
        case BRDF_SAMPLING:
            samplerConfig.surfaceSampler = new CBsdfSampler;
            break;
        case PHOTON_MAP_SAMPLING:
            samplerConfig.surfaceSampler = new CPhotonMapSampler;
            break;
        case CLASSICAL_SAMPLING:
            samplerConfig.surfaceSampler = new CSpecularSampler;
            break;
        default:
            logError("SR CONFIG::initDependentVars", "Wrong sampling mode");
    }

    if ( lightMode == IMPORTANT_LIGHTS ) {
        samplerConfig.neSampler = new ImportantLightSampler;
    } else {
        samplerConfig.neSampler = new UniformLightSampler;
    }


    // Scatter info blocks
    // Storage block
    char storeFlags;

    if ((radianceMethod == nullptr) || (radMode == STORED_NONE) ) {
        storeFlags = NO_COMPONENTS;
    } else {
        if ( radianceMethod->className == PHOTON_MAP ) {
            storeFlags = BSDF_GLOSSY_COMPONENT | BSDF_DIFFUSE_COMPONENT;
        } else {
            storeFlags = BRDF_DIFFUSE_COMPONENT;
        }
    }

    initialReadout = SCATTER;

    switch ( radMode ) {
        case STORED_NONE:
            siStorage.flags = NO_COMPONENTS;
            siStorage.nrSamplesBefore = 0;
            siStorage.nrSamplesAfter = 0;
            break;
        case STORED_DIRECT:
            siStorage.flags = storeFlags;
            siStorage.nrSamplesBefore = 0;
            siStorage.nrSamplesAfter = 0;
            initialReadout = READ_NOW;
            break;
        case STORED_INDIRECT:
        case STORED_PHOTON_MAP:
            siStorage.flags = storeFlags;
            siStorage.nrSamplesBefore = firstDGSamples;
            siStorage.nrSamplesAfter = 0;
            break;
        default:
            logError("SR CONFIG::initDependentVars", "Wrong Rad Mode");
    }

    // Other blocks, this is non storage with optional
    // separation of specular components

    char remainingFlags = BSDF_ALL_COMPONENTS & ~storeFlags;
    int siIndex = 0;

    if ( separateSpecular ) {
        char flags;

        // spec reflection

        if ( reflectionSampling == CLASSICAL_SAMPLING ) {
            flags = (char)(remainingFlags & (BRDF_SPECULAR_COMPONENT |
                                                   BRDF_GLOSSY_COMPONENT)); // Glossy == Specular in classic
        } else {
            flags = (char)(remainingFlags & BRDF_SPECULAR_COMPONENT);
        }

        if ( flags ) {
            siOthers[siIndex].flags = flags;
            siOthers[siIndex].nrSamplesBefore = scatterSamples;
            siOthers[siIndex].nrSamplesAfter = scatterSamples;
            siIndex++;
            remainingFlags = static_cast<char>(remainingFlags & ~flags);
        }

        // Spec transmission
        if ( reflectionSampling == CLASSICAL_SAMPLING ) {
            flags = (char)(remainingFlags & (BTDF_SPECULAR_COMPONENT |
                                                   BTDF_GLOSSY_COMPONENT)); // Glossy == Specular in classic
        } else {
            flags = (char)(remainingFlags & BTDF_SPECULAR_COMPONENT);
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

    if ((reflectionSampling != CLASSICAL_SAMPLING) && (scatterSamples != firstDGSamples) ) {
        char gdFlags = (char)(remainingFlags &
                                          (BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT));
        if ( gdFlags ) {
            siOthers[siIndex].flags = gdFlags;
            siOthers[siIndex].nrSamplesBefore = firstDGSamples;
            siOthers[siIndex].nrSamplesAfter = scatterSamples;
            siIndex++;
            remainingFlags = static_cast<char>(remainingFlags & ~gdFlags);
        }
    }

    if ( reflectionSampling == CLASSICAL_SAMPLING ) {
        // Classical: Diffuse, with no scattering
        char dFlags = (char)(remainingFlags & BSDF_DIFFUSE_COMPONENT);

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
    if ( GLOBAL_lightList ) {
        delete GLOBAL_lightList;
    }

    GLOBAL_lightList = new LightList(lightList, backgroundSampling);

    // Main init the seed config
    seedConfig.init(samplerConfig.maxDepth);
}

#endif

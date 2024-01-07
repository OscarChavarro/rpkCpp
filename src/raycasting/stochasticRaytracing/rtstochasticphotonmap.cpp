#include "common/error.h"
#include "skin/radianceinterfaces.h"
#include "scene/scene.h"
#include "PHOTONMAP/PhotonMapRadiosity.h"
#include "PHOTONMAP/photonmapsampler.h"
#include "raycasting/raytracing/eyesampler.h"
#include "raycasting/raytracing/bsdfsampler.h"
#include "raycasting/raytracing/specularsampler.h"
#include "raycasting/raytracing/lightsampler.h"
#include "raycasting/stochasticRaytracing/rtstochasticphotonmap.h"

CSeed CSeedConfig::m_xorseed;

void SRCONFIG::Init(RTStochastic_State &state) {
    // Copy state options

    samplesPerPixel = state.samplesPerPixel;

    baseSeed = state.baseSeed;

    radMode = state.radMode;

    backgroundIndirect = state.backgroundIndirect;
    backgroundDirect = state.backgroundDirect;
    backgroundSampling = state.backgroundSampling;

    if ( radMode != STORED_NONE ) {
        if ( GLOBAL_radiance_currentRadianceMethodHandle == nullptr ) {
            logError("Stored Radiance", "No radiance method active, using no storage");
            radMode = STORED_NONE;
        } else if ((radMode == STORED_PHOTONMAP) && (GLOBAL_radiance_currentRadianceMethodHandle != &Pmap)) {
            logError("Stored Radiance", "Photonmap method not active, using no storage");
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

    if ( reflectionSampling == CLASSICALSAMPLING ) {
        if ( radMode == STORED_INDIRECT || radMode == STORED_PHOTONMAP ) {
            logError("Classical raytracing",
                     "Incompatible with extended final gather, using storage directly");
            radMode = STORED_DIRECT;
        }
    }

    /*
    if(radMode == STORED_PHOTONMAP)
    {
      Warning("Reflection Sampling",
        "Photonmap method uses special sampling");
      reflectionSampling = BRDFSAMPLING;
    }
    */

    if ( radMode == STORED_PHOTONMAP ) {
        logWarning("Photon map reflection Sampling",
                   "Make sure to use the same sampling (brdf,fresnel) as for photonmap construction");
    }

    scatterSamples = state.scatterSamples;
    if ( state.differentFirstDG ) {
        firstDGSamples = state.firstDGSamples;
    } else {
        firstDGSamples = scatterSamples;
    }

    separateSpecular = state.separateSpecular;

    if ( reflectionSampling == PHOTONMAPSAMPLING )  //radMode == STORED_PHOTONMAP)
    {
        logWarning("Fresnel Specular Sampling",
                   "always uses separate specular");
        separateSpecular = true;  // Always separate specular with photonmap
    }

    samplerConfig.minDepth = state.minPathDepth;
    samplerConfig.maxDepth = state.maxPathDepth;

    screen = new ScreenBuffer(nullptr);
    screen->SetFactor(1.0); // We're storing plain radiance

    InitDependentVars();
}

void SRCONFIG::InitDependentVars() {
    // Sampler configuration
    samplerConfig.pointSampler = new CEyeSampler;
    samplerConfig.dirSampler = new CPixelSampler;

    switch ( reflectionSampling ) {
        case BRDFSAMPLING:
            samplerConfig.surfaceSampler = new CBsdfSampler;
            break;
        case PHOTONMAPSAMPLING:
            samplerConfig.surfaceSampler = new CPhotonMapSampler;
            break;
        case CLASSICALSAMPLING:
            samplerConfig.surfaceSampler = new CSpecularSampler;
            break;
        default:
            logError("SRCONFIG::InitDependentVars", "Wrong sampling mode");
    }

    if ( lightMode == IMPORTANT_LIGHTS ) {
        samplerConfig.neSampler = new CImportantLightSampler;
    } else {
        samplerConfig.neSampler = new CUniformLightSampler;
    }


    // Scatter info blocks

    // Storage block

    BSDFFLAGS storeFlags;

    if ((GLOBAL_radiance_currentRadianceMethodHandle == nullptr) || (radMode == STORED_NONE)) {
        storeFlags = NO_COMPONENTS;
    } else {
        if ( GLOBAL_radiance_currentRadianceMethodHandle == &Pmap ) {
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
        case STORED_PHOTONMAP:
            siStorage.flags = storeFlags;
            siStorage.nrSamplesBefore = firstDGSamples;
            siStorage.nrSamplesAfter = 0;
            break;
        default:
            logError("SRCONFIG::InitDependentVars", "Wrong Rad Mode");
    }

    // Other blocks, this is non storage with optional
    //   separation of specular components

    BSDFFLAGS remainingFlags = BSDF_ALL_COMPONENTS & ~storeFlags;
    int siIndex = 0;

    if ( separateSpecular ) {
        BSDFFLAGS flags;

        // spec reflection

        if ( reflectionSampling == CLASSICALSAMPLING ) {
            flags = remainingFlags & (BRDF_SPECULAR_COMPONENT |
                                      BRDF_GLOSSY_COMPONENT); // Glossy == Specular in classic
        } else {
            flags = remainingFlags & BRDF_SPECULAR_COMPONENT;
        }


        if ( flags ) {
            siOthers[siIndex].flags = flags;
            siOthers[siIndex].nrSamplesBefore = scatterSamples;
            siOthers[siIndex].nrSamplesAfter = scatterSamples;
            siIndex++;
            remainingFlags &= ~flags;
        }

        // spec transmission

        if ( reflectionSampling == CLASSICALSAMPLING ) {
            flags = remainingFlags & (BTDF_SPECULAR_COMPONENT |
                                      BTDF_GLOSSY_COMPONENT); // Glossy == Specular in classic
        } else {
            flags = remainingFlags & BTDF_SPECULAR_COMPONENT;
        }

        if ( flags ) {
            siOthers[siIndex].flags = flags;
            siOthers[siIndex].nrSamplesBefore = scatterSamples;
            siOthers[siIndex].nrSamplesAfter = scatterSamples;
            siIndex++;
            remainingFlags &= ~flags;
        }
    }

    // Glossy or diffuse with different firstDGSamples

    if ((reflectionSampling != CLASSICALSAMPLING) && (scatterSamples != firstDGSamples)) {
        BSDFFLAGS gdFlags = (remainingFlags &
                             (BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT));
        if ( gdFlags ) {
            siOthers[siIndex].flags = gdFlags;
            siOthers[siIndex].nrSamplesBefore = firstDGSamples;
            siOthers[siIndex].nrSamplesAfter = scatterSamples;
            siIndex++;
            remainingFlags &= ~gdFlags;
        }
    }

    if ( reflectionSampling == CLASSICALSAMPLING ) {
        // Classical: Diffuse, with no scattering
        BSDFFLAGS dFlags = (remainingFlags & BSDF_DIFFUSE_COMPONENT);

        if ( dFlags ) {
            siOthers[siIndex].flags = dFlags;
            siOthers[siIndex].nrSamplesBefore = 0;
            siOthers[siIndex].nrSamplesAfter = 0;
            siIndex++;
            remainingFlags &= ~dFlags;
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

    // mainInit the light list
    if ( gLightList ) {
        delete gLightList;
    }
    gLightList = new CLightList(GLOBAL_scene_lightSourcePatches, (bool) backgroundSampling);


    // mainInit the seed config

    seedConfig.Init(samplerConfig.maxDepth);
}

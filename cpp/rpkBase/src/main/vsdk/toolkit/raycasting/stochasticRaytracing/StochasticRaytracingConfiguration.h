#ifndef RT_STOCHASTIC_PHOTON_MAP__
#define RT_STOCHASTIC_PHOTON_MAP__

#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LightList.h"
#include "vsdk/toolkit/raycasting/raytracing/SamplerConfig.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Seed.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/SeedConfig.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/ScatterInfo.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StorageReadout.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

class StochasticRaytracingConfiguration {
  public:
    int samplesPerPixel;

    int nextEventSamples;
    RayTracingLightMode lightMode;

    RayTracingRadMode radMode;

    int scatterSamples;
    int firstDGSamples;
    RayTracingSamplingMode reflectionSampling;
    bool separateSpecular;

    bool backgroundIndirect; // Use background in reflections (indirect)
    bool backgroundDirect; // Use background when no surface is hit (direct)
    bool backgroundSampling; // Use light sampling
    int doFrameCoherent;
    int doCorrelatedSampling;
    long int baseSeed;

    // Independent variables
    ScreenBuffer *screen;
    ToneMappingContext *toneMapOptions;

    // Variables derived from user options
    // All variables must not change during raytracing...
    SamplerConfig samplerConfig;
    SeedConfig seedConfig;

    // Scatter info blocks

    // Scattering info for the part of light
    // transport that is used from storage
    // (Diffuse for getTopLevelPatchRad, Diffuse & Glossy for photon map methods)
    ScatterInfo siStorage;

    // Maximum 1 scattering block per component
    ScatterInfo siOthers[BsdfComponentInfo::BSDF_COMPONENTS];
    int siOthersCount;

    StorageReadout initialReadout;

public:
    void
    init(
        const Camera *defaultCamera,
        const StochasticRayTracingState &state,
        const java::ArrayList<Patch *> *lightList,
        const RadianceMethod *radianceMethod,
        ToneMappingContext *toneMapOptions,
        LightList *&rayTracingLightList);

    // Constructors
    StochasticRaytracingConfiguration(
            Camera *defaultCamera,
            StochasticRayTracingState &state,
            java::ArrayList<Patch *> *lightList,
            RadianceMethod *radianceMethod,
            ToneMappingContext *inToneMapOptions,
            LightList *&rayTracingLightList):
            samplesPerPixel(),
            nextEventSamples(),
            lightMode(),
            radMode(),
            scatterSamples(),
            firstDGSamples(),
            reflectionSampling(),
            separateSpecular(),
            backgroundIndirect(),
            backgroundDirect(),
            backgroundSampling(),
            doFrameCoherent(),
            doCorrelatedSampling(),
            baseSeed(),
            screen(),
            toneMapOptions(),
            samplerConfig(),
            seedConfig(),
            siStorage(),
            siOthers(),
            siOthersCount(),
            initialReadout()
        {
        init(defaultCamera, state, lightList, radianceMethod, inToneMapOptions, rayTracingLightList);
    }

    ~StochasticRaytracingConfiguration() {
        samplerConfig.releaseVars();
    };

private:
    void
    initDependentVars(
        const java::ArrayList<Patch *> *lightList,
        const RadianceMethod *radianceMethod,
        LightList *&rayTracingLightList);
};

#endif

#endif

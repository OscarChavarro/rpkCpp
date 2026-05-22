#ifndef __RT_STOCHASTIC_PHOTON_MAP__
#define __RT_STOCHASTIC_PHOTON_MAP__

#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "java/util/ArrayList.h"
#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "raycasting/raytracing/SamplerConfig.h"
#include "raycasting/stochasticRaytracing/Seed.h"
#include "raycasting/stochasticRaytracing/SeedConfig.h"
#include "raycasting/stochasticRaytracing/ScatterInfo.h"
#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "raycasting/stochasticRaytracing/StorageReadout.h"
#include "tonemap/ToneMappingContext.h"

class StochRaytrConfig {
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
        const ArrayList<Patch *> *lightList,
        const RadianceMethod *radianceMethod,
        ToneMappingContext *toneMapOptions,
        LightList *&rayTracingLightList);

    // Constructors
    StochRaytrConfig(
            Camera *defaultCamera,
            StochasticRayTracingState &state,
            ArrayList<Patch *> *lightList,
            RadianceMethod *radianceMethod,
            ToneMappingContext *inToneMapOptions,
            LightList *&rayTracingLightList):
            samplesPerPixel(),
            nextEventSamples(),
            lightMode(POWER_LIGHTS),
            radMode(STORED_NONE),
            scatterSamples(),
            firstDGSamples(),
            reflectionSampling(BRDF_SAMPLING),
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
            initialReadout(SCATTER)
        {
        init(defaultCamera, state, lightList, radianceMethod, inToneMapOptions, rayTracingLightList);
    }

    ~StochRaytrConfig() {
        samplerConfig.releaseVars();
    };

private:
    void
    initDependentVars(
        const ArrayList<Patch *> *lightList,
        const RadianceMethod *radianceMethod,
        LightList *&rayTracingLightList);
};

#endif

#endif

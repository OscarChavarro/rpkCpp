#ifndef __RT_STOCHASTIC_PHOTON_MAP__
#define __RT_STOCHASTIC_PHOTON_MAP__

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "java/util/ArrayList.h"
#include "raycasting/raytracing/SamplerConfig.h"
#include "raycasting/stochasticRaytracing/CSeed.h"
#include "raycasting/stochasticRaytracing/CSeedConfig.h"
#include "raycasting/stochasticRaytracing/CScatterInfo.h"
#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "raycasting/stochasticRaytracing/StorageReadout.h"

class ToneMappingContext;
class LightList;

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
    CSeedConfig seedConfig;

    // Scatter info blocks

    // Scattering info for the part of light
    // transport that is used from storage
    // (Diffuse for getTopLevelPatchRad, Diffuse & Glossy for GLOBAL_photonMapMethods)
    CScatterInfo siStorage;

    // Maximum 1 scattering block per component
    CScatterInfo siOthers[BSDF_COMPONENTS];
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

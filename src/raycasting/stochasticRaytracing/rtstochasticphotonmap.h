#ifndef __RT_STOCHASTIC_PHOTON_MAP__
#define __RT_STOCHASTIC_PHOTON_MAP__

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "java/util/ArrayList.h"
#include "raycasting/raytracing/samplertools.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracerOptions.h"

/**
SEED Configuration class
*/
class CSeed {
  private:
    unsigned short m_seed[3];
  public:
    unsigned short *GetSeed() { return m_seed; }

    void SetSeed(CSeed seed) {
        const unsigned short *s = seed.GetSeed();
        m_seed[0] = s[0];
        m_seed[1] = s[1];
        m_seed[2] = s[2];
    }

    void SetSeed(const unsigned short seed16v[3]) {
        m_seed[0] = seed16v[0];
        m_seed[1] = seed16v[1];
        m_seed[2] = seed16v[2];
    }

    void SetSeed(unsigned short s0, unsigned short s1, unsigned short s2) {
        m_seed[0] = s0;
        m_seed[1] = s1;
        m_seed[2] = s2;
    }

    void XORSeed(CSeed xOrSeed) {
        const unsigned short *s = xOrSeed.GetSeed();
        m_seed[0] ^= s[0];
        m_seed[1] ^= s[1];
        m_seed[2] ^= s[2];
    }
};

class CSeedConfig {
  private:
    CSeed *m_seeds;
    static CSeed xOrSeed;

  public:
    CSeedConfig() {
        xOrSeed.SetSeed(0xF0, 0x65, 0xDE);
        m_seeds = nullptr;
    }

    void
    clear() {
        if ( m_seeds ) {
            delete[] m_seeds;
        }
    }

    void
    init(int maxDepth) {
        clear();
        m_seeds = new CSeed[maxDepth];
    }

    ~CSeedConfig() {
        clear();
    }

    // Saves the current seed and generates a new seeds based
    // on the current seed
    void
    save(int depth) {
        // Save the seed (supply dummy seed to seed48())
        m_seeds[depth].SetSeed(seed48(m_seeds[depth].GetSeed()));

        //Generate a new seed, dependent on the current seed
        CSeed tmpSeed{};

        tmpSeed.SetSeed(m_seeds[depth]);
        // Fixed xor should do the trick. Note that you can not use
        // the random number generator itself to generate new seeds,
        // because the supplied random numbers *are* the (truncated) seeds
        tmpSeed.XORSeed(xOrSeed);
        // Set the new seed and drand48 once, to be sure
        seed48(tmpSeed.GetSeed());
        drand48();
    }

    // Restores seed for a certain depth
    void Restore(int depth) {
        seed48(m_seeds[depth].GetSeed());
    }
};

/**
CScatterinfo includes information about different scattering properties for different bsdf components
This info is used during scattering, but also when weighting or reading storage decisions must be made
*/
class CScatterInfo {
  public:
    // The components under consideration
    char flags;
    // Spawning factor if no 'flags' bounce was made before
    int nrSamplesBefore;
    // Spawning factor after at least one 'flags' bounce
    int nrSamplesAfter;

    // Some utility functions

    // Were 'flags' last used in the bounce in 'node'
    bool
    DoneThisBounce(const SimpleRaytracingPathNode *node) const {
        return (node->m_usedComponents == flags);
    }

    // Were 'flags' used at some previous point in the path
    bool
    DoneSomePreviousBounce(const SimpleRaytracingPathNode *node) const {
        return ((node->m_accUsedComponents & flags) == flags);
    }
};

// Next enum is needed to track readout of storage.
enum StorageReadout {
    SCATTER,
    READ_NOW
};

class StochasticRaytracingConfiguration {
public:
    // *** user options

    int samplesPerPixel;

    int nextEventSamples;
    RayTracingLightMode lightMode;

    RayTracingRadMode radMode;

    int scatterSamples;
    int firstDGSamples;
    RayTracingSamplingMode reflectionSampling;
    bool separateSpecular;

    bool backgroundIndirect;      // use background in reflections (indirect)
    bool backgroundDirect;        // use background when no surface is hit (direct)
    bool backgroundSampling;      // use light sampling

    // *** independent variables
    ScreenBuffer *screen;

    // *** variables derived from user options
    // *** all variables must not change during raytracing...
    CSamplerConfig samplerConfig;
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
    void init(Camera *defaultCamera, RayTracingStochasticState &state, java::ArrayList<Patch *> *lightList, RadianceMethod *radianceMethod);

    // Constructors
    StochasticRaytracingConfiguration(
        Camera *defaultCamera,
        RayTracingStochasticState &state,
        java::ArrayList<Patch *> *lightList,
        RadianceMethod *radianceMethod):
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
            screen(),
            samplerConfig(),
            seedConfig(),
            siStorage(),
            siOthers(),
            siOthersCount(),
            initialReadout()
        {
        init(defaultCamera, state, lightList, radianceMethod);
    }

    ~StochasticRaytracingConfiguration() {
        samplerConfig.releaseVars();
    };

private:
    void initDependentVars(java::ArrayList<Patch *> *lightList, RadianceMethod *radianceMethod);
};

#endif

#endif

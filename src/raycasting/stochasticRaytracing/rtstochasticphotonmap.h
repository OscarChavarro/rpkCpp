#ifndef __RPK_RTSTOCHASTICPHOTONMAP__
#define __RPK_RTSTOCHASTICPHOTONMAP__

#include "raycasting/raytracing/samplertools.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracerOptions.h"

/*** SEED Configuration class ***/
class CSeed {
private:
    unsigned short m_seed[3];
public:
    unsigned short *GetSeed() { return m_seed; }

    void SetSeed(CSeed seed) {
        unsigned short *s = seed.GetSeed();
        m_seed[0] = s[0];
        m_seed[1] = s[1];
        m_seed[2] = s[2];
    }

    void SetSeed(unsigned short seed16v[3]) {
        m_seed[0] = seed16v[0];
        m_seed[1] = seed16v[1];
        m_seed[2] = seed16v[2];
    }

    void SetSeed(unsigned short s0, unsigned short s1, unsigned short s2) {
        m_seed[0] = s0;
        m_seed[1] = s1;
        m_seed[2] = s2;
    }

    void XORSeed(CSeed xorseed) {
        unsigned short *s = xorseed.GetSeed();
        m_seed[0] ^= s[0];
        m_seed[1] ^= s[1];
        m_seed[2] ^= s[2];
    }
};

class CSeedConfig {
private:
    CSeed *m_seeds;
    static CSeed m_xorseed;

public:

    // Constructor
    CSeedConfig() {
        m_xorseed.SetSeed(0xF0, 0x65, 0xDE);
        m_seeds = 0;
    }

    void Clear() {
        if ( m_seeds ) {
            delete[] m_seeds;
        }
    }

    void Init(int maxdepth) {
        Clear();
        m_seeds = new CSeed[maxdepth];
    }

    ~CSeedConfig() {
        Clear();
    }

    // Saves the current seed and generates a new seeds based
    // on the current seed
    void Save(int depth) {
        // Save the seed (supply dummy seed to seed48())
        m_seeds[depth].SetSeed(seed48(m_seeds[depth].GetSeed()));

        //Generate a new seed, dependent on the current seed
        CSeed tmpSeed;

        tmpSeed.SetSeed(m_seeds[depth]);
        // Fixed xor should do the trick. Note that you can not use
        // the random number generator itself to generate new seeds,
        // because the supplied random numbers *are* the (truncated) seeds
        tmpSeed.XORSeed(m_xorseed);
        // Set the new seed and drand48 once, to be sure
        seed48(tmpSeed.GetSeed());
        drand48();
    }

    // Restores seed for a certain depth
    void Restore(int depth) {
        seed48(m_seeds[depth].GetSeed());
    }
};

// CScatterinfo includes information about different
// scattering properties for different bsdf components
// This info is used during scattering, but also
// when weighting or reading storage decisions
// must be made.

class CScatterInfo {
public:
    // The components under consideration
    BSDFFLAGS flags;
    // Spawning factor if no 'flags' bounce was made before
    int nrSamplesBefore;
    // Spawning factor after at least one 'flags' bounce
    int nrSamplesAfter;

    // Some utility functions

    // Were 'flags' last used in the bounce in 'node'
    bool DoneThisBounce(CPathNode *node) { return (node->m_usedComponents == flags); }

    // Were 'flags' used at some point in the path
    bool DoneSomeBounce(CPathNode *node) {
        return (((node->m_accUsedComponents | node->m_usedComponents) & flags) == flags);
    }

    // Were 'flags' used at some previous point in the path
    bool DoneSomePreviousBounce(CPathNode *node) { return ((node->m_accUsedComponents & flags) == flags); }
};

// Next enum is needed to track readout of storage.
enum SRREADOUT {
    SCATTER, READ_NOW
};

/*** Stochastic raytracing configuration structure ***/
class SRCONFIG {
public:
    // *** user options

    int samplesPerPixel;

    int nextEventSamples;
    RTSLIGHT_MODE lightMode;

    RTSRAD_MODE radMode;

    int scatterSamples;
    int firstDGSamples;
    RTSSAMPLING_MODE reflectionSampling;
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
    long baseSeed;

    // Scatter info blocks

    // Scattering info for the part of light
    // transport that is used from storage
    // (Diffuse for getTopLevelPatchRad, Diffuse & Glossy for GLOBAL_photonMapMethods)
    CScatterInfo siStorage;

    // Maximum 1 scattering block per component
    CScatterInfo siOthers[BSDFCOMPONENTS];
    int siOthersCount;

    SRREADOUT initialReadout;

public:
    void Init(RTStochastic_State &state);

    // Constructors
    SRCONFIG() {}

    SRCONFIG(RTStochastic_State &state) { Init(state); }

    ~SRCONFIG(){
        samplerConfig.ReleaseVars();
    };

private:
    void InitDependentVars();
};

#endif

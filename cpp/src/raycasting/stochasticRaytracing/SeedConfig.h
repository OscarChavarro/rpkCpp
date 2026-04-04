#ifndef __STOCHASTIC_RAYTRACING_CSEED_CONFIG__
#define __STOCHASTIC_RAYTRACING_CSEED_CONFIG__

#include <cstdlib>

#include "raycasting/stochasticRaytracing/Seed.h"

class SeedConfig {
  private:
    Seed *m_seeds;
    static Seed xOrSeed;

  public:
    SeedConfig() {
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
        m_seeds = new Seed[maxDepth];
    }

    ~SeedConfig() {
        clear();
    }

    // Saves the current seed and generates a new seeds based
    // on the current seed
    void
    save(int depth) {
        // Save the seed (supply dummy seed to seed48())
        m_seeds[depth].SetSeed(seed48(m_seeds[depth].GetSeed()));

        //Generate a new seed, dependent on the current seed
        Seed tmpSeed{};

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

#endif

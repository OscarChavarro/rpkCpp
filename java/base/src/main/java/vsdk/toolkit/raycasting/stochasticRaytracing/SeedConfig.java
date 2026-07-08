package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.common.Random;

public class SeedConfig {
    private Seed[] m_seeds;
    private static final Seed xOrSeed = new Seed();

    public SeedConfig() {
        xOrSeed.SetSeed(0xF0, 0x65, 0xDE);
        m_seeds = null;
    }

    public void
    clear() {
        m_seeds = null;
    }

    public void
    init(int maxDepth) {
        clear();
        m_seeds = new Seed[maxDepth];
        for (int i = 0; i < maxDepth; i++) {
            m_seeds[i] = new Seed();
        }
    }

    // Saves the current seed and generates a new seeds based
    // on the current seed
    public void
    save(int depth) {
        if (m_seeds == null || depth < 0 || depth >= m_seeds.length) {
            return;
        }

        // Save the seed (supply dummy seed to seed48())
        Seed current = m_seeds[depth];
        current.SetSeed(Random.seed48(current.GetSeed()));

        //Generate a new seed, dependent on the current seed
        Seed tmpSeed = new Seed();

        tmpSeed.SetSeed(current);
        // Fixed xor should do the trick. Note that you can not use
        // the random number generator itself to generate new seeds,
        // because the supplied random numbers *are* the (truncated) seeds
        tmpSeed.XORSeed(xOrSeed);

        // Set the new seed and drand48 once, to be sure
        Random.seed48(tmpSeed.GetSeed());
        Random.drand48();
    }

    // Restores seed for a certain depth
    public void Restore(int depth) {
        if (m_seeds == null || depth < 0 || depth >= m_seeds.length) {
            return;
        }
        Random.seed48(m_seeds[depth].GetSeed());
    }
}

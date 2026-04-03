package vsdk.toolkit.raycasting.stochasticRaytracing;

import java.util.Random;

public class CSeedConfig {
    private CSeed[] m_seeds;
    private static final CSeed xOrSeed = new CSeed();

    private static Random randomFromSeed(CSeed seed) {
        short[] s = seed.GetSeed();
        long merged = (((long)s[0] & 0xFFFFL) << 32)
            ^ (((long)s[1] & 0xFFFFL) << 16)
            ^ ((long)s[2] & 0xFFFFL);
        return new Random(merged);
    }

    public CSeedConfig() {
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
        m_seeds = new CSeed[maxDepth];
        for (int i = 0; i < maxDepth; i++) {
            m_seeds[i] = new CSeed();
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
        CSeed current = m_seeds[depth];

        //Generate a new seed, dependent on the current seed
        CSeed tmpSeed = new CSeed();

        tmpSeed.SetSeed(current);
        // Fixed xor should do the trick. Note that you can not use
        // the random number generator itself to generate new seeds,
        // because the supplied random numbers *are* the (truncated) seeds
        tmpSeed.XORSeed(xOrSeed);

        Random random = randomFromSeed(tmpSeed);
        short s0 = (short)random.nextInt(0x10000);
        short s1 = (short)random.nextInt(0x10000);
        short s2 = (short)random.nextInt(0x10000);
        current.SetSeed(new short[] {s0, s1, s2});

        random.nextDouble();
    }

    // Restores seed for a certain depth
    public void Restore(int depth) {
        // Java port keeps seeds only; explicit global RNG restore is not required.
    }
}

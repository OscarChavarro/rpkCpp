package vsdk.toolkit.raycasting.stochasticRaytracing;

/**
SEED Configuration class
*/
public class CSeed {
    private final short[] m_seed;

    public CSeed() {
        m_seed = new short[3];
    }

    public short[] GetSeed() {
        return m_seed;
    }

    public void SetSeed(CSeed seed) {
        short[] s = seed.GetSeed();
        m_seed[0] = s[0];
        m_seed[1] = s[1];
        m_seed[2] = s[2];
    }

    public void SetSeed(short[] seed16v) {
        m_seed[0] = seed16v[0];
        m_seed[1] = seed16v[1];
        m_seed[2] = seed16v[2];
    }

    public void SetSeed(int s0, int s1, int s2) {
        m_seed[0] = (short)s0;
        m_seed[1] = (short)s1;
        m_seed[2] = (short)s2;
    }

    public void XORSeed(CSeed xOrSeed) {
        short[] s = xOrSeed.GetSeed();
        m_seed[0] ^= s[0];
        m_seed[1] ^= s[1];
        m_seed[2] ^= s[2];
    }
}

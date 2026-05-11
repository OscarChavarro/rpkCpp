#ifndef STOCHASTIC_RAYTRACING_CSEED__
#define STOCHASTIC_RAYTRACING_CSEED__

/**
Configuration class
*/
class Seed {
  private:
    unsigned short m_seed[3];
  public:
    unsigned short *GetSeed() {
        return m_seed;
    }

    void SetSeed(Seed seed) {
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

    void XORSeed(Seed xOrSeed) {
        const unsigned short *s = xOrSeed.GetSeed();
        m_seed[0] ^= s[0];
        m_seed[1] ^= s[1];
        m_seed[2] ^= s[2];
    }
};

#endif

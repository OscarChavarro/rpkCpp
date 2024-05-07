#ifndef __REFRACTION_INDEX__
#define __REFRACTION_INDEX__

class RefractionIndex {
  private:
    float nr;
    float ni;

  public:
    float complexToGeometricRefractionIndex() const;

    inline float
    getNr() const {
        return nr;
    }

    inline float
    getNi() const {
        return ni;
    }

    inline void
    set(float inNr, float inNi) {
        nr = inNr;
        ni = inNi;
    }
};

#endif

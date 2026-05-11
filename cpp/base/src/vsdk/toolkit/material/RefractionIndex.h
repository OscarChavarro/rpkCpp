#ifndef REFRACTION_INDEX__
#define REFRACTION_INDEX__

class RefractionIndex {
  private:
    float nr;
    float ni;

  public:
    float complexToGeometricRefractionIndex() const;

    float getNr() const;
    float getNi() const;
    void set(float inNr, float inNi);
};

inline float
RefractionIndex::getNr() const {
    return nr;
}

inline float
RefractionIndex::getNi() const {
    return ni;
}

inline void
RefractionIndex::set(float inNr, float inNi) {
    nr = inNr;
    ni = inNi;
}

#endif

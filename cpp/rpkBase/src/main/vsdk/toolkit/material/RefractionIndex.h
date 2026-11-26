#ifndef __REFRACTION_INDEX__
#define __REFRACTION_INDEX__

class RefractionIndex {
  private:
    double nr;
    double ni;

  public:
    double complexToGeometricRefractionIndex() const;

    double getNr() const;
    double getNi() const;
    void set(double inNr, double inNi);
};

inline double
RefractionIndex::getNr() const {
    return nr;
}

inline double
RefractionIndex::getNi() const {
    return ni;
}

inline void
RefractionIndex::set(double inNr, double inNi) {
    nr = inNr;
    ni = inNi;
}

#endif

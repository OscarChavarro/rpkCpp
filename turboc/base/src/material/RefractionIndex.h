#ifndef __REFRACTION_INDEX__
#define __REFRACTION_INDEX__

#include "common/VSDK.h"

class RefractionIndex {
  private:
    float nr;
    float ni;

  public:
    float cmplxTGmtrcRfrctIndx() const;

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

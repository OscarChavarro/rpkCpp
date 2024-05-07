#ifndef __MATERIAL__
#define __MATERIAL__

#include <cstdio>

#include "material/PhongEmittanceDistributionFunction.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "material/RayHit.h"

class Material {
  private:
    PhongEmittanceDistributionFunction *edf; // Emittance distribution function

  public:
    char *name; // Material name
    PhongBidirectionalScatteringDistributionFunction *bsdf; // Reflection and transmission together
    bool sided; // True for 1-sided surface, false for 2-sided, see mgf docs

    explicit Material(
        const char *inName,
        PhongEmittanceDistributionFunction *edf,
        PhongBidirectionalScatteringDistributionFunction *bsdf,
        bool sided);
    virtual ~Material();

    inline PhongEmittanceDistributionFunction *
    getEdf() {
        return edf;
    }
};

#endif

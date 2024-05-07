#ifndef __MATERIAL__
#define __MATERIAL__

#include <cstdio>

#include "material/PhongEmittanceDistributionFunction.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "material/RayHit.h"

class Material {
  public:
    char *name; // Material name
    PhongEmittanceDistributionFunction *edf; // Emittance distribution function
    PhongBidirectionalScatteringDistributionFunction *bsdf; // Reflection and transmission together
    bool sided; // True for 1-sided surface, false for 2-sided, see mgf docs

    explicit Material(
        const char *inName,
        PhongEmittanceDistributionFunction *edf,
        PhongBidirectionalScatteringDistributionFunction *bsdf,
        bool sided);
    virtual ~Material();
};

#endif

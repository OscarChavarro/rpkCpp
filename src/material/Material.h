#ifndef __MATERIAL__
#define __MATERIAL__

#include <cstdio>

#include "material/PhongEmittanceDistributionFunction.h"
#include "material/BidirectionalScatteringDistributionFunction.h"
#include "material/RayHit.h"

class Material {
  public:
    char *name; // Material name
    PhongEmittanceDistributionFunction *edf; // Emittance distribution function
    BidirectionalScatteringDistributionFunction *bsdf; // Reflection and transmission together
    int sided; // 1 for 1-sided surface, 0 for 2-sided, see mgf docs

    Material();
    virtual ~Material();
};

extern Material *
materialCreate(
        const char *inName,
        PhongEmittanceDistributionFunction *edf,
        BidirectionalScatteringDistributionFunction *bsdf,
        int sided);

#endif

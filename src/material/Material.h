#ifndef __MATERIAL__
#define __MATERIAL__

#include <cstdio>

#include "material/PhongEmittanceDistributionFunctions.h"
#include "material/bsdf.h"
#include "material/hit.h"

class Material {
  public:
    char *name; // Material name
    PhongEmittanceDistributionFunctions *edf; // Emittance distribution function
    BSDF *bsdf; // Reflection and transmission together
    int sided; // 1 for 1-sided surface, 0 for 2-sided, see mgf docs

    Material(const char *inName);
    virtual ~Material();
};

extern Material GLOBAL_material_defaultMaterial;

extern Material *
materialCreate(
    char *name,
    PhongEmittanceDistributionFunctions *edf,
    BSDF *bsdf,
    int sided);

#endif

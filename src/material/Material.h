#ifndef __MATERIAL__
#define __MATERIAL__

#include <cstdio>

#include "material/edf.h"
#include "material/bsdf.h"
#include "material/hit.h"

class Material {
  public:
    const char *name; // Material name
    EDF *edf; // Emittance distribution function
    BSDF *bsdf; // Reflection and transmission together
    int sided; // 1 for 1-sided surface, 0 for 2-sided, see mgf docs
    const char *radiance_data;
};

extern Material GLOBAL_material_defaultMaterial;

extern Material *materialCreate(const char *name,
                                EDF *edf, BSDF *bsdf,
                                int sided);

#endif

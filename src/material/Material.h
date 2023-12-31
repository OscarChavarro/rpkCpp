#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include <cstdio>

#include "material/edf.h"
#include "material/bsdf.h"
#include "material/hit.h"

class Material {
  public:
    const char *name;        /* material name */
    EDF *edf;        /* emittance distribution function */
    BSDF *bsdf;          /* reflection and transmission together */
    int sided;        /* 1 for 1-sided surface, 0 for 2-sided, see mgf docs */
    void *radiance_data;
};

extern Material GLOBAL_material_defaultMaterial;

extern Material *MaterialCreate(const char *name,
                                EDF *edf, BSDF *bsdf,
                                int sided);

extern void MaterialDestroy(Material *material);

#endif

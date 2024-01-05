#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include <cstdio>

#include "material/edf.h"
#include "material/bsdf.h"
#include "material/hit.h"

class MATERIAL {
  public:
    const char *name;        /* material name */
    EDF *edf;        /* emittance distribution function */
    BSDF *bsdf;          /* reflection and transmission together */
    int sided;        /* 1 for 1-sided surface, 0 for 2-sided, see mgf docs */
    void *radiance_data;
};

extern MATERIAL defaultMaterial;

extern MATERIAL *MaterialCreate(const char *name,
                                EDF *edf, BSDF *bsdf,
                                int sided);

extern void MaterialDestroy(MATERIAL *material);

#endif

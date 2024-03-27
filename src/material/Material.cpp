#include <cstring>
#include <cstdlib>

#include "material/Material.h"
#include "material/bsdf.h"

Material GLOBAL_material_defaultMaterial = {
        "(default)",
        nullptr, nullptr,
        0 // Sided
};

Material *
materialCreate(const char *name,
               PhongEmittanceDistributionFunctions *edf, BSDF *bsdf,
               int sided) {
    Material *m = (Material *)malloc(sizeof(Material));
    m->name = (char *)malloc(strlen(name) + 1);
    snprintf((char *)m->name, strlen(name) + 1, "%s", name);
    m->sided = sided;
    m->edf = edf;
    m->bsdf = bsdf;
    m->radiance_data = nullptr;
    return m;
}

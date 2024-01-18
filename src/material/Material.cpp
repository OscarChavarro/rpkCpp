#include <cstring>
#include <cstdlib>

#include "material/Material.h"
#include "material/bsdf.h"

Material GLOBAL_material_defaultMaterial = {
        "(default)",
        (EDF *) nullptr, (BSDF *)nullptr,
        0    /* sided */
};

Material *
MaterialCreate(const char *name,
                         EDF *edf, BSDF *bsdf,
                         int sided) {
    Material *m = (Material *)malloc(sizeof(Material));
    m->name = (char *)malloc(strlen(name) + 1);
    snprintf((char *)m->name, strlen(name) + 1, "%s", name);
    m->sided = sided;
    m->edf = edf;
    m->bsdf = bsdf;
    m->radiance_data = (void *)nullptr;
    return m;
}

void
MaterialDestroy(Material *material) {
    if ( material->name ) {
        free((void *)material->name);
    }
    if ( material->edf ) {
        EdfDestroy(material->edf);
    }
    if ( material->bsdf ) {
        BsdfDestroy(material->bsdf);
    }
    free(material);
}

#include <cstring>
#include <cstdlib>

#include "material/Material.h"
#include "material/bsdf.h"

MATERIAL defaultMaterial = {
        "(default)",
        (EDF *) nullptr, (BSDF *)nullptr,
        0    /* sided */
};

MATERIAL *
MaterialCreate(const char *name,
                         EDF *edf, BSDF *bsdf,
                         int sided) {
    MATERIAL *m = (MATERIAL *)malloc(sizeof(MATERIAL));
    m->name = (char *)malloc(strlen(name) + 1);
    sprintf((char *)m->name, "%s", name);
    m->sided = sided;
    m->edf = edf;
    m->bsdf = bsdf;
    m->radiance_data = (void *)nullptr;
    return m;
}

void
MaterialDestroy(MATERIAL *material) {
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

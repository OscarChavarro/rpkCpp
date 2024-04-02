#include <cstring>
#include <cstdlib>

#include "material/Material.h"
#include "material/bsdf.h"

Material GLOBAL_material_defaultMaterial;

Material::Material(): name(), edf(), bsdf(), sided(0) {
    name = new char[10];
    strcpy(name, "(default)");
}

Material::~Material() {
    //delete name;
}

Material *
materialCreate(
    char *name,
    PhongEmittanceDistributionFunctions *edf,
    BSDF *bsdf,
    int sided)
{
    Material *m = new Material();
    delete m->name;
    m->name = new char[strlen(name) + 1];
    snprintf((char *)m->name, strlen(name) + 1, "%s", name);
    m->sided = sided;
    m->edf = edf;
    m->bsdf = bsdf;
    return m;
}

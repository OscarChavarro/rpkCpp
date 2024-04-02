#include <cstring>
#include <cstdlib>

#include "material/Material.h"
#include "material/bsdf.h"

Material GLOBAL_material_defaultMaterial("(default)");

Material::Material(const char *inName): name(), edf(), bsdf(), sided(0) {
    name = new char[strlen(inName) + 1];
    snprintf((char *)name, strlen(inName) + 1, "%s", inName);
}

Material::~Material() {
    delete[] name;
    delete bsdf;
    delete edf;
}

Material *
materialCreate(
    char *name,
    PhongEmittanceDistributionFunctions *edf,
    BSDF *bsdf,
    int sided)
{
    Material *m = new Material(name);
    m->sided = sided;
    m->edf = edf;
    m->bsdf = bsdf;
    return m;
}

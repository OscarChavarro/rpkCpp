#include <cstring>

#include "material/Material.h"
#include "material/bsdf.h"

Material::Material(): name(), edf(), bsdf(), sided(0) {
    name = new char[10];
    strcpy(name, "(default)");
}

Material::~Material() {
    delete[] name;
    delete bsdf;
    delete edf;
}

Material *
materialCreate(
    const char *inName,
    PhongEmittanceDistributionFunction *edf,
    BSDF *bsdf,
    int sided)
{
    Material *m = new Material();
    delete[] m->name;
    m->name = new char[strlen(inName) + 1];
    strcpy(m->name, inName);
    m->sided = sided;
    m->edf = edf;
    m->bsdf = bsdf;
    return m;
}

#include <cstring>

#include "material/Material.h"

Material::Material(
    const char *inName,
    PhongEmittanceDistributionFunction *inEdf,
    PhongBidirectionalScatteringDistributionFunction *inBsdf,
    const bool inSided)
{
    name = new char[strlen(inName) + 1];
    strcpy(name, inName);
    sided = inSided;
    edf = inEdf;
    bsdf = inBsdf;
}

Material::~Material() {
    delete[] name;
    delete bsdf;
    delete edf;
}

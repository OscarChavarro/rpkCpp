#include <string.h>

#include "vsdk/toolkit/material/RpkMaterial.h"

RpkMaterial::RpkMaterial(
    const char *inName,
    PhongEmitDistFunc *inEdf,
    PhongBidirScattDistFunc *inBsdf,
    const bool inSided)
{
    name = new char[strlen(inName) + 1];
    strcpy(name, inName);
    sided = inSided;
    edf = inEdf;
    bsdf = inBsdf;
}

RpkMaterial::~RpkMaterial() {
    delete[] name;
    delete bsdf;
    delete edf;
}

#include <cstring>
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/material/RpkMaterial.h"

RpkMaterial::RpkMaterial(
    const char *inName,
    const PhongLobe *emission,
    const bool hasBsdf,
    const PhongLobe *reflection,
    const PhongLobe *transmission,
    Texture *inTexture,
    const float refractionIndexReal,
    const float refractionIndexImaginary,
    const bool inSided):
    name(nullptr),
    sided(inSided),
    texture(inTexture),
    surfaceMaterial(nullptr),
    edf(nullptr),
    bsdf(nullptr)
{
    name = new char[strlen(inName) + 1];
    strcpy(name, inName);
    surfaceMaterial = new PhongMaterial(
        inName, reflection, transmission, emission, texture, inSided, refractionIndexReal, refractionIndexImaginary);
    if ( emission != nullptr ) {
        if ( !emission->getNonDiffuse().isBlack() ) {
            Logger::reportMessage("PhongEmittanceDistributionFunction", Logger::WARNING, "phongEdfCreate", "Non-diffuse light sources not yet implemented");
        }
        edf = new PhongEmittanceDistributionFunction(surfaceMaterial);
    }
    if ( hasBsdf ) {
        bsdf = new PhongBidirectionalScatteringDistributionFunction(surfaceMaterial);
    }
}

RpkMaterial::~RpkMaterial() {
    delete bsdf;
    delete edf;
    delete surfaceMaterial;
    delete texture;
    delete[] name;
}

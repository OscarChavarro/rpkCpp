#include <cstring>

#include "vsdk/toolkit/material/RpkMaterial.h"

RpkMaterial::RpkMaterial(
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
    surfaceMaterial = buildSurfaceMaterial(inName, inEdf, inBsdf, inSided);
}

RpkMaterial::~RpkMaterial() {
    delete surfaceMaterial;
    delete[] name;
    delete bsdf;
    delete edf;
}

/**
Conversion boundary to the VITRAL material: the colors are the same ColorRgb
values and the exponents the same floats, so the integrated quantities are
bit-identical to the ones of the Phong EDF and BSDF.
*/
const PhongMaterial *
RpkMaterial::buildSurfaceMaterial(
    const char *name,
    const PhongEmittanceDistributionFunction *edf,
    const PhongBidirectionalScatteringDistributionFunction *bsdf,
    const bool sided)
{
    const PhongLobe *reflection = nullptr;
    const PhongLobe *transmission = nullptr;
    const Texture *texture = nullptr;
    if ( bsdf != nullptr ) {
        const PhongBidirectionalReflectanceDistributionFunction *brdf = bsdf->getBrdf();
        if ( brdf != nullptr ) {
            reflection = new PhongLobe(brdf->getKd(), brdf->getKs(), brdf->getNs());
        }
        const PhongBidirectionalTransmittanceDistributionFunction *btdf = bsdf->getBtdf();
        if ( btdf != nullptr ) {
            transmission = new PhongLobe(btdf->getKd(), btdf->getKs(), btdf->getNs());
        }
        texture = bsdf->getTexture();
    }
    const PhongLobe *emission = nullptr;
    if ( edf != nullptr ) {
        emission = new PhongLobe(edf->getKd(), edf->getKs(), edf->getNs());
    }
    return new PhongMaterial(name, reflection, transmission, emission, texture, sided);
}

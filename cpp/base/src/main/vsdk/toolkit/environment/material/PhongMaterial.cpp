#include <cstring>
#include "java/lang/Float.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/environment/material/PhongMaterial.h"

static bool
isFiniteColor(const ColorRgb &color) {
    return java::Float::isFinite(static_cast<float>((color.getR() + color.getG() + color.getB()) / 3.0));
}

static const PhongLobe *
copyLobe(const PhongLobe *lobe) {
    return lobe != nullptr ? new PhongLobe(*lobe) : nullptr;
}

PhongMaterial::PhongMaterial(
    const char *inName,
    const PhongLobe *inReflection,
    const PhongLobe *inTransmission,
    const PhongLobe *inEmission,
    const Texture *inTexture,
    const bool inSided):
    name(nullptr),
    sided(inSided),
    reflection(inReflection),
    transmission(inTransmission),
    emission(inEmission),
    texture(inTexture)
{
    const char *source = inName != nullptr ? inName : "";
    name = new char[strlen(source) + 1];
    strcpy(name, source);
}

PhongMaterial::~PhongMaterial() {
    delete[] name;
    delete reflection;
    delete transmission;
    delete emission;
}

ColorRgb
PhongMaterial::scatteredPower(int components, const Vector3Df *textureCoordinate) const {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;

    if ( texture != nullptr && (components & TEXTURED_COMPONENT) ) {
        if ( textureCoordinate == nullptr ) {
            Logger::reportMessage("PhongMaterial", Logger::WARNING, "scatteredPower", "Couldn't get texture coordinates");
        } else {
            const ColorRgb textureColor = texture->evaluateColor(textureCoordinate->x(), textureCoordinate->y());
            r += textureColor.getR();
            g += textureColor.getG();
            b += textureColor.getB();
        }
        components &= ~TEXTURED_COMPONENT;
    }

    if ( reflection != nullptr ) {
        const ColorRgb reflectance = reflection->integrated(ScatteringComponent::reflectionComponents(components));
        if ( !isFiniteColor(reflectance) ) {
            Logger::reportMessage("PhongMaterial", Logger::FATAL_ERROR, "scatteredPower", "Reflectance is not finite");
        }
        r += reflectance.getR();
        g += reflectance.getG();
        b += reflectance.getB();
    }

    if ( transmission != nullptr ) {
        const ColorRgb transmittance = transmission->integrated(ScatteringComponent::transmissionComponents(components));
        if ( !isFiniteColor(transmittance) ) {
            Logger::reportMessage("PhongMaterial", Logger::FATAL_ERROR, "scatteredPower", "Transmittance is not finite");
        }
        r += transmittance.getR();
        g += transmittance.getG();
        b += transmittance.getB();
    }

    return ColorRgb(r, g, b);
}

ColorRgb
PhongMaterial::emittance(const int components) const {
    if ( emission == nullptr ) {
        return ColorRgb(0.0, 0.0, 0.0);
    }
    return emission->integrated(components);
}

Material *
PhongMaterial::copy() const {
    return new PhongMaterial(name, copyLobe(reflection), copyLobe(transmission), copyLobe(emission), texture, sided);
}

Material *
PhongMaterial::translate(Vector3Dd * /*vector*/) {
    return this;
}

Material *
PhongMaterial::rotate(Vector3Dd * /*vector*/) {
    return this;
}

Material *
PhongMaterial::scale(Vector3Dd * /*vector*/) {
    return this;
}

#include <stdio.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
SimpleMaterial::SimpleMaterial()
    : ambient(ColorRgb(0.1, 0.1, 0.1)),
      diffuse(ColorRgb(0.9, 0.5, 0.5)),
      specular(ColorRgb(1.0, 1.0, 1.0)),
      emission(ColorRgb(0.0, 0.0, 0.0)),
      transmittance(ColorRgb(0.0, 0.0, 0.0)),
      doubleSided(TRUE),
      reflectionCoefficient(0.0),
      refractionCoefficient(0.0),
      indexOfRefraction(1.0),
      name("VSDK_default_material"),
      opacity(1.0),
      phongExponent(128.0)
{
}

SimpleMaterial::SimpleMaterial(const SimpleMaterial& m)
    : ambient(m.ambient),
      diffuse(m.diffuse),
      specular(m.specular),
      emission(m.emission),
      transmittance(m.transmittance),
      doubleSided(m.doubleSided),
      reflectionCoefficient(m.reflectionCoefficient),
      refractionCoefficient(m.refractionCoefficient),
      indexOfRefraction(m.indexOfRefraction),
      name(m.name),
      opacity(m.opacity),
      phongExponent(m.phongExponent)
{
}

SimpleMaterial::SimpleMaterial(
    const String& inName,
    const ColorRgb& inAmbient,
    const ColorRgb& inDiffuse,
    const ColorRgb& inSpecular,
    const ColorRgb& inEmission,
    const ColorRgb& inTransmittance,
    BOOLEAN inDoubleSided,
    double inReflectionCoefficient,
    double inRefractionCoefficient,
    double inIndexOfRefraction,
    double inOpacity,
    double inPhongExponent)
    : ambient(inAmbient),
      diffuse(inDiffuse),
      specular(inSpecular),
      emission(inEmission),
      transmittance(inTransmittance),
      doubleSided(inDoubleSided),
      reflectionCoefficient(inReflectionCoefficient),
      refractionCoefficient(inRefractionCoefficient),
      indexOfRefraction(inIndexOfRefraction),
      name(inName),
      opacity(inOpacity),
      phongExponent(inPhongExponent)
{
}

SimpleMaterial::SimpleMaterial(
    const String& inName,
    const ColorRgb& inAmbient,
    const ColorRgb& inDiffuse,
    const ColorRgb& inSpecular,
    BOOLEAN inDoubleSided,
    double inReflectionCoefficient,
    double inRefractionCoefficient,
    double inOpacity,
    double inPhongExponent)
    : ambient(inAmbient),
      diffuse(inDiffuse),
      specular(inSpecular),
      emission(ColorRgb(0.0, 0.0, 0.0)),
      transmittance(ColorRgb(0.0, 0.0, 0.0)),
      doubleSided(inDoubleSided),
      reflectionCoefficient(inReflectionCoefficient),
      refractionCoefficient(inRefractionCoefficient),
      indexOfRefraction(1.0),
      name(inName),
      opacity(inOpacity),
      phongExponent(inPhongExponent)
{
}

const String& SimpleMaterial::getName() const { return name; }

SimpleMaterial SimpleMaterial::withName(const String& n) const { return SimpleMaterial(n, ambient, diffuse, specular, emission, transmittance, doubleSided, reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, phongExponent); }
SimpleMaterial SimpleMaterial::withAmbient(const ColorRgb& a) const { return SimpleMaterial(name, a, diffuse, specular, emission, transmittance, doubleSided, reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, phongExponent); }
SimpleMaterial SimpleMaterial::withDiffuse(const ColorRgb& d) const { return SimpleMaterial(name, ambient, d, specular, emission, transmittance, doubleSided, reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, phongExponent); }
SimpleMaterial SimpleMaterial::withSpecular(const ColorRgb& s) const { return SimpleMaterial(name, ambient, diffuse, s, emission, transmittance, doubleSided, reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, phongExponent); }
SimpleMaterial SimpleMaterial::withEmission(const ColorRgb& e) const { return SimpleMaterial(name, ambient, diffuse, specular, e, transmittance, doubleSided, reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, phongExponent); }
SimpleMaterial SimpleMaterial::withTransmittance(const ColorRgb& t) const { return SimpleMaterial(name, ambient, diffuse, specular, emission, t, doubleSided, reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, phongExponent); }
SimpleMaterial SimpleMaterial::withPhongExponent(double p) const { return SimpleMaterial(name, ambient, diffuse, specular, emission, transmittance, doubleSided, reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, p); }
SimpleMaterial SimpleMaterial::withReflectionCoefficient(double kr) const { return SimpleMaterial(name, ambient, diffuse, specular, emission, transmittance, doubleSided, kr, refractionCoefficient, indexOfRefraction, opacity, phongExponent); }
SimpleMaterial SimpleMaterial::withRefractionCoefficient(double kr) const { return SimpleMaterial(name, ambient, diffuse, specular, emission, transmittance, doubleSided, reflectionCoefficient, kr, indexOfRefraction, opacity, phongExponent); }
SimpleMaterial SimpleMaterial::withIndexOfRefraction(double ior) const { return SimpleMaterial(name, ambient, diffuse, specular, emission, transmittance, doubleSided, reflectionCoefficient, refractionCoefficient, ior, opacity, phongExponent); }
SimpleMaterial SimpleMaterial::withOpacity(double a) const { return SimpleMaterial(name, ambient, diffuse, specular, emission, transmittance, doubleSided, reflectionCoefficient, refractionCoefficient, indexOfRefraction, a, phongExponent); }
SimpleMaterial SimpleMaterial::withDoubleSided(BOOLEAN b) const { return SimpleMaterial(name, ambient, diffuse, specular, emission, transmittance, b, reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, phongExponent); }

BOOLEAN SimpleMaterial::isDoubleSided() const { return doubleSided; }

ColorRgb SimpleMaterial::getAmbient() const { return ColorRgb(ambient); }
const ColorRgb& SimpleMaterial::getAmbientReference() const { return ambient; }
ColorRgb SimpleMaterial::getDiffuse() const { return ColorRgb(diffuse); }
const ColorRgb& SimpleMaterial::getDiffuseReference() const { return diffuse; }
ColorRgb SimpleMaterial::getSpecular() const { return ColorRgb(specular); }
const ColorRgb& SimpleMaterial::getSpecularReference() const { return specular; }
ColorRgb SimpleMaterial::getEmission() const { return ColorRgb(emission); }
const ColorRgb& SimpleMaterial::getEmissionReference() const { return emission; }
ColorRgb SimpleMaterial::getTransmittance() const { return ColorRgb(transmittance); }
const ColorRgb& SimpleMaterial::getTransmittanceReference() const { return transmittance; }

double SimpleMaterial::getPhongExponent() const { return phongExponent; }
double SimpleMaterial::getReflectionCoefficient() const { return reflectionCoefficient; }
double SimpleMaterial::getRefractionCoefficient() const { return refractionCoefficient; }
double SimpleMaterial::getIndexOfRefraction() const { return indexOfRefraction; }
double SimpleMaterial::getOpacity() const { return opacity; }

String SimpleMaterial::toString() const
{
    char buf[512];
    snprintf(buf, sizeof(buf),
        "SimpleMaterial [%s]:\n"
        "  - Specular (%.6g, %.6g, %.6g)\n"
        "  - Diffuse (%.6g, %.6g, %.6g)\n"
        "  - Ambient (%.6g, %.6g, %.6g)\n"
        "  - Phong exponent: %.6g\n"
        "  - %s\n\n",
        name.c_str(),
        specular.r(), specular.g(), specular.b(),
        diffuse.r(), diffuse.g(), diffuse.b(),
        ambient.r(), ambient.g(), ambient.b(),
        phongExponent,
        doubleSided ? "Double sided" : "Single sided");
    return String(buf);
}

Material *SimpleMaterial::copy() const { return new SimpleMaterial(*this); }
Material *SimpleMaterial::translate(Vector3Dd * /*vector*/) { return this; }
Material *SimpleMaterial::rotate(Vector3Dd * /*vector*/) { return this; }
Material *SimpleMaterial::scale(Vector3Dd * /*vector*/) { return this; }

const Class SimpleMaterial::CLASS = { "SimpleMaterial", &Entity::CLASS };

const Class *
SimpleMaterial::getClass() const
{
    return &CLASS;
}

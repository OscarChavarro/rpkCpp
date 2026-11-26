#ifndef __SIMPLE_MATERIAL__
#define __SIMPLE_MATERIAL__

#include "vsdk/toolkit/common/Entity.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/material/Material.h"

class SimpleMaterial : public Entity, public Material {
  private:
    ColorRgb ambient;
    ColorRgb diffuse;
    ColorRgb specular;
    ColorRgb emission;
    ColorRgb transmittance;
    bool doubleSided;
    double reflectionCoefficient;
    double refractionCoefficient;
    double indexOfRefraction;
    java::String name;
    double opacity;
    double phongExponent;

  public:
    SimpleMaterial();
    SimpleMaterial(const SimpleMaterial& m);
    SimpleMaterial(
        const java::String& name,
        const ColorRgb& ambient,
        const ColorRgb& diffuse,
        const ColorRgb& specular,
        const ColorRgb& emission,
        const ColorRgb& transmittance,
        bool doubleSided,
        double reflectionCoefficient,
        double refractionCoefficient,
        double indexOfRefraction,
        double opacity,
        double phongExponent);

    SimpleMaterial(
        const java::String& name,
        const ColorRgb& ambient,
        const ColorRgb& diffuse,
        const ColorRgb& specular,
        bool doubleSided,
        double reflectionCoefficient,
        double refractionCoefficient,
        double opacity,
        double phongExponent);
    virtual ~SimpleMaterial() {}

    const java::String& getName() const;

    SimpleMaterial withName(const java::String& n) const;
    SimpleMaterial withAmbient(const ColorRgb& a) const;
    SimpleMaterial withDiffuse(const ColorRgb& d) const;
    SimpleMaterial withSpecular(const ColorRgb& s) const;
    SimpleMaterial withEmission(const ColorRgb& e) const;
    SimpleMaterial withTransmittance(const ColorRgb& t) const;
    SimpleMaterial withPhongExponent(double p) const;
    SimpleMaterial withReflectionCoefficient(double kr) const;
    SimpleMaterial withRefractionCoefficient(double kr) const;
    SimpleMaterial withIndexOfRefraction(double ior) const;
    SimpleMaterial withOpacity(double a) const;
    SimpleMaterial withDoubleSided(bool b) const;

    bool isDoubleSided() const;

    ColorRgb getAmbient() const;
    const ColorRgb& getAmbientReference() const;
    ColorRgb getDiffuse() const;
    const ColorRgb& getDiffuseReference() const;
    ColorRgb getSpecular() const;
    const ColorRgb& getSpecularReference() const;
    ColorRgb getEmission() const;
    const ColorRgb& getEmissionReference() const;
    ColorRgb getTransmittance() const;
    const ColorRgb& getTransmittanceReference() const;

    double getPhongExponent() const;
    double getReflectionCoefficient() const;
    double getRefractionCoefficient() const;
    double getIndexOfRefraction() const;
    double getOpacity() const;

    java::String toString() const;

    Material *copy() const override;
    Material *translate(Vector3Dd *vector) override;
    Material *rotate(Vector3Dd *vector) override;
    Material *scale(Vector3Dd *vector) override;
};

#endif

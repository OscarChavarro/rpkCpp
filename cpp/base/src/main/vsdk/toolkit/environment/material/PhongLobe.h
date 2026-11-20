#ifndef __PHONG_LOBE__
#define __PHONG_LOBE__

#include "vsdk/toolkit/common/color/ColorRgb.h"

/**
Immutable Phong lobe as MGF materials define one: a diffuse color Kd, a
non-diffuse color Ks and a Phong exponent Ns. The non-diffuse part counts as
the SPECULAR component when Ns reaches LOWEST_SPECULAR_EXPONENT and as the
GLOSSY component below it. The same lobe describes reflection, transmission
or emission; PhongMaterial composes them.

The exponent is stored in single precision, as RenderPark stores it, so that
the glossy/specular threshold classifies every material the same way.
*/
class PhongLobe {
  private:
    const ColorRgb diffuse;
    const ColorRgb nonDiffuse;
    const float exponent;

  public:
    static const float LOWEST_SPECULAR_EXPONENT;

    PhongLobe(const ColorRgb &inDiffuse, const ColorRgb &inNonDiffuse, float inExponent);
    PhongLobe(const PhongLobe &other);
    PhongLobe &operator=(const PhongLobe &) = delete;

    const ColorRgb &getDiffuse() const;
    const ColorRgb &getNonDiffuse() const;
    float getExponent() const;
    bool isSpecular() const;

    /**
    Sum of the colors of the requested components (ScatteringComponent
    DIFFUSE, GLOSSY, SPECULAR): the integrated reflectance, transmittance
    or emittance of the lobe. Accumulated in double precision starting from
    zero, diffuse first.
    */
    ColorRgb integrated(int components) const;
};

inline const ColorRgb &
PhongLobe::getDiffuse() const {
    return diffuse;
}

inline const ColorRgb &
PhongLobe::getNonDiffuse() const {
    return nonDiffuse;
}

inline float
PhongLobe::getExponent() const {
    return exponent;
}

inline bool
PhongLobe::isSpecular() const {
    return exponent >= LOWEST_SPECULAR_EXPONENT;
}

#endif

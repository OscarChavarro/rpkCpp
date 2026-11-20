#ifndef __PHONG_MATERIAL__
#define __PHONG_MATERIAL__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/environment/material/Material.h"
#include "vsdk/toolkit/environment/material/PhongLobe.h"
#include "vsdk/toolkit/environment/material/ScatteringComponent.h"
#include "vsdk/toolkit/environment/material/Texture.h"

/**
Surface material made of Phong lobes, as MGF scenes describe materials: an
optional reflection lobe, an optional transmission lobe, an optional emission
lobe, an optional color texture that replaces the diffuse reflection, a name
and a sidedness (one-sided surfaces scatter and emit on their front only).

It answers the integrated quantities a radiosity solver consumes:
scatteredPower (reflectance plus transmittance by component) and emittance
by component. Directional evaluation and sampling are not part of it yet.

Ownership and copies. The material owns its lobes and its name; the texture
is shared, read-only and owned by whoever loaded it, and must outlive every
material that refers to it. Construction adopts the lobes passed in (nullptr
for an absent lobe). The material is immutable once built, so one instance
can be read from any number of threads. copy() is a deep copy of the lobes and
the name that shares the texture. The geometric transforms of Material do not
change a surface material and return the material itself.
*/
class PhongMaterial : public Material {
  private:
    char *name;
    const bool sided;
    const PhongLobe *reflection;
    const PhongLobe *transmission;
    const PhongLobe *emission;
    const Texture *texture;

  public:
    /**
    The texture color replaces this component of the scattered power
    when a texture is present.
    */
    static const int TEXTURED_COMPONENT = ScatteringComponent::REFLECTION_DIFFUSE;

    PhongMaterial(
        const char *inName,
        const PhongLobe *inReflection,
        const PhongLobe *inTransmission,
        const PhongLobe *inEmission,
        const Texture *inTexture,
        bool inSided);
    virtual ~PhongMaterial();
    PhongMaterial(const PhongMaterial &) = delete;
    PhongMaterial &operator=(const PhongMaterial &) = delete;

    const char *getName() const;
    bool isSided() const;
    bool isTextured() const;
    bool hasEmission() const;
    const PhongLobe *getReflection() const;
    const PhongLobe *getTransmission() const;
    const PhongLobe *getEmission() const;
    const Texture *getTexture() const;

    /**
    Integrated reflectance plus transmittance of the requested components
    (ScatteringComponent mask covering both lobes). When the material is
    textured and TEXTURED_COMPONENT is requested, the texture color at
    textureCoordinate (its x and y) is added first and that component is
    then left out of the reflection lobe. A null textureCoordinate means
    the coordinate is unknown: a warning is reported and the texture adds
    nothing.
    */
    ColorRgb scatteredPower(int components, const Vector3Df *textureCoordinate) const;

    /**
    Integrated emittance of the requested components (ScatteringComponent
    DIFFUSE, GLOSSY, SPECULAR), black when the material does not emit.
    */
    ColorRgb emittance(int components) const;

    virtual Material *copy() const;
    virtual Material *translate(Vector3Dd *vector);
    virtual Material *rotate(Vector3Dd *vector);
    virtual Material *scale(Vector3Dd *vector);
};

inline const char *
PhongMaterial::getName() const {
    return name;
}

inline bool
PhongMaterial::isSided() const {
    return sided;
}

inline bool
PhongMaterial::isTextured() const {
    return texture != nullptr;
}

inline bool
PhongMaterial::hasEmission() const {
    return emission != nullptr;
}

inline const PhongLobe *
PhongMaterial::getReflection() const {
    return reflection;
}

inline const PhongLobe *
PhongMaterial::getTransmission() const {
    return transmission;
}

inline const PhongLobe *
PhongMaterial::getEmission() const {
    return emission;
}

inline const Texture *
PhongMaterial::getTexture() const {
    return texture;
}

#endif

#ifndef __PHONG_MATERIAL__
#define __PHONG_MATERIAL__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/material/Material.h"
#include "vsdk/toolkit/environment/material/PhongLobe.h"
#include "vsdk/toolkit/environment/material/ScatteringComponent.h"
#include "vsdk/toolkit/environment/material/SurfaceShadingPoint.h"
#include "vsdk/toolkit/environment/material/Texture.h"

/**
Surface material made of Phong lobes, as MGF scenes describe materials: an
optional reflection lobe, an optional transmission lobe, an optional emission
lobe, an optional color texture that replaces the diffuse reflection, a name
and a sidedness (one-sided surfaces scatter and emit on their front only).

It answers the integrated quantities a radiosity solver consumes:
scatteredPower (reflectance plus transmittance by component) and emittance
by component, and the directional ones a raytracer consumes: evaluation,
sampling from supplied variates and the density of a direction, for the
scattering (reflection, transmission and the texture, which scatters as a
diffuse reflector whose albedo is the texture color) and for the emission
(diffuse only). Their conventions are PhongLobe's; the arithmetic is
RenderPark's, so results are bit-identical to its split BSDF and Phong EDF.

Media. Transmission needs the refraction index on both sides of the
surface: the directional scattering functions take the materials of the
medium the ray comes from (inMedium) and of the one beyond (outMedium), and
read their getRefractionIndex(). A null medium in evaluation and sampling
has the index 0 (RenderPark's unset index); in densityOf it has the vacuum
index 1. A material without transmission lobe has the vacuum index.

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
    const double storedRefractionIndexReal;
    const double storedRefractionIndexImaginary;

    ColorRgb textureColor(const SurfaceShadingPoint &point) const;
    void samplingProbabilities(
        const SurfaceShadingPoint &point,
        int components,
        double *texturePart,
        double *reflectionPart,
        double *transmissionPart,
        int *reflectionComponents,
        int *transmissionComponents) const;

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

    /** As above, with the complex refraction index of the transmission lobe */
    PhongMaterial(
        const char *inName,
        const PhongLobe *inReflection,
        const PhongLobe *inTransmission,
        const PhongLobe *inEmission,
        const Texture *inTexture,
        bool inSided,
        double inRefractionIndexReal,
        double inRefractionIndexImaginary);
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
    ColorRgb scatteredPower(int components, const Vector3Dd *textureCoordinate) const;

    /**
    Integrated emittance of the requested components (ScatteringComponent
    DIFFUSE, GLOSSY, SPECULAR), black when the material does not emit.
    */
    ColorRgb emittance(int components) const;

    /**
    Real part of the refraction index seen by a ray that crosses the
    surface: the transmission lobe's, 1 (vacuum) without one
    */
    double getRefractionIndex() const;

    /** Imaginary part of the transmission lobe's refraction index, 0 without one */
    double getRefractionIndexImaginary() const;

    /** Stored refraction index, also without transmission lobe (for persistence) */
    double getStoredRefractionIndexReal() const;
    double getStoredRefractionIndexImaginary() const;

    /**
    Scattering (BSDF) value for the requested components (ScatteringComponent
    mask over both lobes). The transmission part is added only when the
    material has a reflection lobe, as RenderPark does.
    */
    ColorRgb scatteringEvaluate(
        const SurfaceShadingPoint &point,
        const PhongMaterial *inMedium,
        const PhongMaterial *outMedium,
        const Vector3Dd &in,
        const Vector3Dd &out,
        int components) const;

    /**
    Samples a scattered direction: the first variate chooses the texture,
    the reflection or the transmission in proportion to their average
    albedos (and absorption, with Russian roulette), then the chosen part
    samples the direction; *density is the density of the direction summed
    over all parts, 0 when no direction was produced
    */
    Vector3Dd scatteringSample(
        const SurfaceShadingPoint &point,
        const PhongMaterial *inMedium,
        const PhongMaterial *outMedium,
        const Vector3Dd &in,
        bool russianRoulette,
        int components,
        double x1,
        double x2,
        double *density) const;

    /**
    Density with which scatteringSample (without Russian roulette)
    produces `out`, and the survival probability of the roulette
    */
    void scatteringDensity(
        const SurfaceShadingPoint &point,
        const PhongMaterial *inMedium,
        const PhongMaterial *outMedium,
        const Vector3Dd &in,
        const Vector3Dd &out,
        int components,
        double *density,
        double *survival) const;

    /**
    scatteringEvaluate of each requested component on its own, into
    componentValues[i] for component 1 << i (black for the others, six
    entries); returns their sum
    */
    ColorRgb scatteringEvaluateComponents(
        const SurfaceShadingPoint &point,
        const PhongMaterial *inMedium,
        const PhongMaterial *outMedium,
        const Vector3Dd &in,
        const Vector3Dd &out,
        int components,
        ColorRgb *componentValues) const;

    /**
    Emitted radiance into `out` of the requested emission components (only
    the diffuse one emits: Kd / pi), and, when density is not null, the
    density of `out` under emissionSample. Black when the point or its
    normal is unknown (reported), or `out` is below the surface.
    */
    ColorRgb emissionEvaluate(
        const SurfaceShadingPoint *point,
        const Vector3Dd &out,
        int components,
        double *density) const;

    /**
    Samples an emitted direction (cosine distributed around the normal when
    the diffuse component is requested; (0, 0, 1) otherwise or when the
    normal is unknown); the radiance along it and its density go to the
    non-null pointers
    */
    Vector3Dd emissionSample(
        const SurfaceShadingPoint *point,
        int components,
        double xi1,
        double xi2,
        ColorRgb *radiance,
        double *density) const;

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

inline double
PhongMaterial::getRefractionIndex() const {
    return transmission != nullptr ? storedRefractionIndexReal : 1.0;
}

inline double
PhongMaterial::getRefractionIndexImaginary() const {
    return transmission != nullptr ? storedRefractionIndexImaginary : 0.0;
}

inline double
PhongMaterial::getStoredRefractionIndexReal() const {
    return storedRefractionIndexReal;
}

inline double
PhongMaterial::getStoredRefractionIndexImaginary() const {
    return storedRefractionIndexImaginary;
}

#endif

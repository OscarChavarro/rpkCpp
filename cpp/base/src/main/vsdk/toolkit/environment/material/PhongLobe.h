#ifndef __PHONG_LOBE__
#define __PHONG_LOBE__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"

/**
Immutable Phong lobe as MGF materials define one: a diffuse color Kd, a
non-diffuse color Ks and a Phong exponent Ns. The non-diffuse part counts as
the SPECULAR component when Ns reaches LOWEST_SPECULAR_EXPONENT and as the
GLOSSY component below it. The same lobe describes reflection, transmission
or emission; PhongMaterial composes them.

The exponent is stored in single precision, as RenderPark stores it, so that
the glossy/specular threshold classifies every material the same way.

Directional functions (RenderPark's modified Phong model, which is symmetric
and energy conserving): with kd = Kd / pi and ks = Ks (Ns + 2) / (2 pi), the
lobe evaluates to kd + ks cos(a)^Ns, a being the angle between the outgoing
direction and the ideal (mirror or refracted) one. Kd and Ks are the energy
scattered for perpendicular incidence, so Kd + Ks <= 1 conserves energy.
Sampling picks the diffuse part with probability avg(Kd) / (avg(Kd) +
avg(Ks)), cosine distributed around the normal, or the non-diffuse part,
distributed as cos^Ns around the ideal direction (density (Ns + 1) cos^Ns /
(2 pi): it differs from the (Ns + 2) of the evaluation); the density of a
direction is the mixture of both. With Russian roulette the first variate
also decides absorption: the direction survives with probability avg(Kd) +
avg(Ks), and the density returned is then not divided by it.

Conventions: `in` and `out` both leave the surface: `in` points back to
where the incoming light (or importance) comes from, `out` to where it
goes; `normal` is the shading normal, on either side (the densities use
the normal turned to the side of `in`; the transmission density swaps the
indices when in . normal < 0). Densities are per unit solid angle; the
diffuse transmission density keeps RenderPark's sign, cos(theta) / pi with
theta measured from the normal on the side of `in`, so it is negative; the caller supplies the variates x1 and x2. A density of 0 means no
direction was produced (absorption or a direction below the surface).
The arithmetic, its precision and its order are RenderPark's, so results are
bit-identical to RenderPark's BRDF and BTDF.
*/
class PhongLobe {
  private:
    const ColorRgb diffuse;
    const ColorRgb nonDiffuse;
    const float exponent;
    const float averageDiffuse;
    const float averageNonDiffuse;

    double sampledDiffuse(int components) const;
    double sampledNonDiffuse(int components) const;

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

    /** Reflected radiance factor (BRDF value) of the requested components */
    ColorRgb reflectionEvaluate(const Vector3Df &in, const Vector3Df &out, const Vector3Df &normal, int components) const;

    /** Samples a reflected direction; its density goes to *density */
    Vector3Df reflectionSample(
        const Vector3Df &in,
        const Vector3Df &normal,
        bool russianRoulette,
        int components,
        double x1,
        double x2,
        double *density) const;

    /**
    Density with which reflectionSample (without Russian roulette) produces
    `out`, and the survival probability of the roulette; the normal is first
    turned to the side of `in`
    */
    void reflectionDensity(
        const Vector3Df &in,
        const Vector3Df &out,
        const Vector3Df &normal,
        int components,
        double *density,
        double *survival) const;

    /**
    Transmitted radiance factor (BTDF value) of the requested components,
    with the real refraction indices on the side of `in` and beyond
    */
    ColorRgb transmissionEvaluate(
        float inIndex,
        float outIndex,
        const Vector3Df &in,
        const Vector3Df &out,
        const Vector3Df &normal,
        int components) const;

    /** Samples a transmitted direction; its density goes to *density */
    Vector3Df transmissionSample(
        float inIndex,
        float outIndex,
        const Vector3Df &in,
        const Vector3Df &normal,
        bool russianRoulette,
        int components,
        double x1,
        double x2,
        double *density) const;

    /** Density with which transmissionSample produces `out`, and the roulette survival */
    void transmissionDensity(
        float inIndex,
        float outIndex,
        const Vector3Df &in,
        const Vector3Df &out,
        const Vector3Df &normal,
        int components,
        double *density,
        double *survival) const;
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

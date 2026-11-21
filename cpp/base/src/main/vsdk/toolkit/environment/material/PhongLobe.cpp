#include <cmath>
#include "java/lang/Math.h"
#include "vsdk/toolkit/environment/material/PhongLobe.h"
#include "vsdk/toolkit/environment/material/ScatteringComponent.h"
#include "vsdk/toolkit/environment/material/ScatteringFrame.h"

using namespace scatteringArithmetic;

namespace {

// Scattered power below which nothing is sampled (RenderPark's EPSILON)
const double NO_SCATTERING = 1e-6;

float
averageOf(const ColorRgb &color) {
    return static_cast<float>((color.getR() + color.getG() + color.getB()) / 3.0);
}

}

const float PhongLobe::LOWEST_SPECULAR_EXPONENT = 250.0F;

PhongLobe::PhongLobe(const ColorRgb &inDiffuse, const ColorRgb &inNonDiffuse, const float inExponent):
    diffuse(inDiffuse),
    nonDiffuse(inNonDiffuse),
    exponent(inExponent),
    averageDiffuse(averageOf(inDiffuse)),
    averageNonDiffuse(averageOf(inNonDiffuse))
{
}

PhongLobe::PhongLobe(const PhongLobe &other):
    diffuse(other.diffuse),
    nonDiffuse(other.nonDiffuse),
    exponent(other.exponent),
    averageDiffuse(other.averageDiffuse),
    averageNonDiffuse(other.averageNonDiffuse)
{
}

/** Average diffuse color when the diffuse component is requested, else 0 */
double
PhongLobe::sampledDiffuse(const int components) const {
    return (components & ScatteringComponent::DIFFUSE) ? averageDiffuse : 0.0;
}

/** Average non-diffuse color when its component (glossy or specular) is requested, else 0 */
double
PhongLobe::sampledNonDiffuse(const int components) const {
    const int nonDiffuseComponent = isSpecular() ? ScatteringComponent::SPECULAR : ScatteringComponent::GLOSSY;
    return (components & nonDiffuseComponent) ? averageNonDiffuse : 0.0;
}

ColorRgb
PhongLobe::integrated(const int components) const {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;

    if ( components & ScatteringComponent::DIFFUSE ) {
        r += diffuse.getR();
        g += diffuse.getG();
        b += diffuse.getB();
    }

    const int nonDiffuseComponent = isSpecular() ? ScatteringComponent::SPECULAR : ScatteringComponent::GLOSSY;
    if ( components & nonDiffuseComponent ) {
        r += nonDiffuse.getR();
        g += nonDiffuse.getG();
        b += nonDiffuse.getB();
    }

    return ColorRgb(r, g, b);
}

ColorRgb
PhongLobe::reflectionEvaluate(const Vector3Df &in, const Vector3Df &out, const Vector3Df &normal, const int components) const {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    const Vector3Df inReversed = scale(-1.0F, in);

    if ( dot(out, normal) < 0 ) {
        // Below the surface: transmission, not reflection
        return ColorRgb(0.0, 0.0, 0.0);
    }

    if ( (components & ScatteringComponent::DIFFUSE) && (averageDiffuse > 0.0) ) {
        r += M_1_PI * diffuse.getR();
        g += M_1_PI * diffuse.getG();
        b += M_1_PI * diffuse.getB();
    }

    const int nonDiffuseComponent = isSpecular() ? ScatteringComponent::SPECULAR : ScatteringComponent::GLOSSY;
    if ( (components & nonDiffuseComponent) && (averageNonDiffuse > 0.0) ) {
        const Vector3Df idealReflected = ScatteringFrame::idealReflection(inReversed, normal);
        const float cosAlpha = dot(idealReflected, out);
        if ( cosAlpha > 0 ) {
            float factor = java::Math::pow(cosAlpha, exponent); // cos(a)^Ns
            factor *= (exponent + 2.0F) / (2.0F * static_cast<float>(M_PI)); // Ks -> ks
            r += factor * nonDiffuse.getR();
            g += factor * nonDiffuse.getG();
            b += factor * nonDiffuse.getB();
        }
    }
    return ColorRgb(r, g, b);
}

Vector3Df
PhongLobe::reflectionSample(
    const Vector3Df &in,
    const Vector3Df &normal,
    const bool russianRoulette,
    const int components,
    double x1,
    const double x2,
    double *density) const
{
    const Vector3Df inReversed = scale(-1.0F, in);
    *density = 0;

    const double localDiffuse = sampledDiffuse(components);
    const double localNonDiffuse = sampledNonDiffuse(components);
    const double scatteredPower = localDiffuse + localNonDiffuse;
    if ( scatteredPower < NO_SCATTERING ) {
        return Vector3Df(0.0F, 0.0F, 0.0F);
    }

    if ( russianRoulette ) {
        if ( x1 > scatteredPower ) {
            // Absorption
            return Vector3Df(0.0F, 0.0F, 0.0F);
        }
        x1 /= scatteredPower;
    }

    const Vector3Df idealDirection = ScatteringFrame::idealReflection(inReversed, normal);
    // The first variate chooses the part, then is rescaled into [0, 1) within it
    const bool diffusePart = x1 < (localDiffuse / scatteredPower);
    const double xPart = diffusePart
        ? x1 / (localDiffuse / scatteredPower)
        : (x1 - (localDiffuse / scatteredPower)) / (localNonDiffuse / scatteredPower);
    double diffuseDensity = 0.0;
    double nonDiffuseDensity = 0.0;
    // Diffuse part cosine distributed around the normal; glossy or specular
    // part cos^Ns distributed around the mirror direction
    const Vector3Df newDirection = diffusePart
        ? ScatteringFrame(normal).sampleCosine(xPart, x2, &diffuseDensity)
        : ScatteringFrame(idealDirection).samplePhong(exponent, xPart, x2, &nonDiffuseDensity);

    if ( diffusePart ) {
        const float cosAlpha = dot(idealDirection, newDirection);
        if ( cosAlpha > 0 ) {
            nonDiffuseDensity = (exponent + 1.0) * java::Math::pow(cosAlpha, exponent) / (2.0 * M_PI);
        }
    } else {
        const double cosTheta = dot(normal, newDirection);
        if ( cosTheta <= 0 ) {
            return newDirection;
        }
        diffuseDensity = cosTheta / M_PI;
    }

    *density = localDiffuse * diffuseDensity + localNonDiffuse * nonDiffuseDensity;
    if ( !russianRoulette ) {
        *density /= scatteredPower;
    }
    return newDirection;
}

void
PhongLobe::reflectionDensity(
    const Vector3Df &in,
    const Vector3Df &out,
    const Vector3Df &normal,
    const int components,
    double *density,
    double *survival) const
{
    const Vector3Df inReversed = scale(-1.0F, in);
    *density = 0;
    *survival = 0;

    // The normal on the side of 'in'
    const double cosIn = dot(in, normal);
    const Vector3Df goodNormal = cosIn >= 0 ? normal : scale(-1.0F, normal);
    const double cosTheta = dot(goodNormal, out);
    if ( cosTheta < 0 ) {
        return;
    }

    const double localDiffuse = sampledDiffuse(components);
    const double localNonDiffuse = sampledNonDiffuse(components);
    const double scatteredPower = localDiffuse + localNonDiffuse;
    if ( scatteredPower < NO_SCATTERING ) {
        return;
    }

    double diffuseDensity = 0.0;
    if ( averageDiffuse > 0 ) {
        diffuseDensity = cosTheta / M_PI;
    }

    double nonDiffuseDensity = 0.0;
    if ( averageNonDiffuse > 0 ) {
        const Vector3Df idealDirection = ScatteringFrame::idealReflection(inReversed, goodNormal);
        const double cosAlpha = dot(idealDirection, out);
        if ( cosAlpha > 0 ) {
            nonDiffuseDensity = (exponent + 1.0) * java::Math::pow(cosAlpha, static_cast<double>(exponent)) / (2.0 * M_PI);
        }
    }

    // RenderPark weighs both parts by their averages here, requested or not
    *density = (averageDiffuse * diffuseDensity + averageNonDiffuse * nonDiffuseDensity) / scatteredPower;
    *survival = scatteredPower;
}

ColorRgb
PhongLobe::transmissionEvaluate(
    const float inIndex,
    const float outIndex,
    const Vector3Df &in,
    const Vector3Df &out,
    const Vector3Df &normal,
    const int components) const
{
    const Vector3Df inReversed = scale(-1.0F, in);
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;

    if ( (components & ScatteringComponent::DIFFUSE) && (averageDiffuse > 0) ) {
        // Diffuse transmission always crosses the boundary
        const bool isReflection = (dot(normal, out) >= 0);
        if ( !isReflection ) {
            r += M_1_PI * diffuse.getR();
            g += M_1_PI * diffuse.getG();
            b += M_1_PI * diffuse.getB();
        }
    }

    const int nonDiffuseComponent = isSpecular() ? ScatteringComponent::SPECULAR : ScatteringComponent::GLOSSY;
    if ( (components & nonDiffuseComponent) && (averageNonDiffuse > 0) ) {
        bool totalInternalReflection;
        const Vector3Df idealRefracted = ScatteringFrame::idealRefraction(
            inReversed, normal, inIndex, outIndex, &totalInternalReflection);
        const float cosAlpha = dot(idealRefracted, out);
        if ( cosAlpha > 0 ) {
            float factor = java::Math::pow(cosAlpha, exponent); // cos(a)^Ns
            factor *= (exponent + 2.0F) / (2.0F * static_cast<float>(M_PI)); // Ks -> ks
            r += factor * nonDiffuse.getR();
            g += factor * nonDiffuse.getG();
            b += factor * nonDiffuse.getB();
        }
    }
    return ColorRgb(r, g, b);
}

Vector3Df
PhongLobe::transmissionSample(
    const float inIndex,
    const float outIndex,
    const Vector3Df &in,
    const Vector3Df &normal,
    const bool russianRoulette,
    const int components,
    double x1,
    const double x2,
    double *density) const
{
    const Vector3Df inReversed = scale(-1.0F, in);
    *density = 0;

    const double localDiffuse = sampledDiffuse(components);
    const double localNonDiffuse = sampledNonDiffuse(components);
    const double scatteredPower = localDiffuse + localNonDiffuse;
    if ( scatteredPower < NO_SCATTERING ) {
        return Vector3Df(0.0F, 0.0F, 0.0F);
    }

    if ( russianRoulette ) {
        if ( x1 > scatteredPower ) {
            // Absorption
            return Vector3Df(0.0F, 0.0F, 0.0F);
        }
        x1 /= scatteredPower;
    }

    bool totalInternalReflection;
    const Vector3Df idealDirection = ScatteringFrame::idealRefraction(
        inReversed, normal, inIndex, outIndex, &totalInternalReflection);
    const Vector3Df inverseNormal = scale(-1.0F, normal);
    // The first variate chooses the part, then is rescaled into [0, 1) within it
    const bool diffusePart = x1 < (localDiffuse / scatteredPower);
    const double xPart = diffusePart
        ? x1 / (localDiffuse / scatteredPower)
        : (x1 - (localDiffuse / scatteredPower)) / (localNonDiffuse / scatteredPower);
    double diffuseDensity = 0.0;
    double nonDiffuseDensity = 0.0;
    // Diffuse part cosine distributed in the transmitted hemisphere; glossy
    // or specular part cos^Ns distributed around the refracted direction
    const Vector3Df newDirection = diffusePart
        ? ScatteringFrame(inverseNormal).sampleCosine(xPart, x2, &diffuseDensity)
        : ScatteringFrame(idealDirection).samplePhong(exponent, xPart, x2, &nonDiffuseDensity);

    if ( diffusePart ) {
        const float cosAlpha = dot(idealDirection, newDirection);
        if ( cosAlpha > 0 ) {
            nonDiffuseDensity = (exponent + 1.0) * java::Math::pow(cosAlpha, exponent) / (2.0 * M_PI);
        }
    } else {
        // A direction on the side of the normal is taken as total internal reflection
        const double cosTheta = dot(normal, newDirection);
        if ( cosTheta > 0 ) {
            diffuseDensity = cosTheta / M_PI;
        }
    }

    *density = localDiffuse * diffuseDensity + localNonDiffuse * nonDiffuseDensity;
    if ( !russianRoulette ) {
        *density /= scatteredPower;
    }
    return newDirection;
}

void
PhongLobe::transmissionDensity(
    const float inIndex,
    const float outIndex,
    const Vector3Df &in,
    const Vector3Df &out,
    const Vector3Df &normal,
    const int components,
    double *density,
    double *survival) const
{
    const Vector3Df inReversed = scale(-1.0F, in);
    *density = 0;
    *survival = 0;

    // The normal on the side of 'in'
    const double cosIn = dot(in, normal);
    const Vector3Df goodNormal = cosIn >= 0 ? normal : scale(-1.0F, normal);
    const double cosTheta = dot(goodNormal, out);

    // Diffuse transmission only for a direction that crossed the surface
    const double localDiffuse = ((components & ScatteringComponent::DIFFUSE) && (cosTheta < 0)) ? averageDiffuse : 0.0;
    const double localNonDiffuse = sampledNonDiffuse(components);
    const double scatteredPower = localDiffuse + localNonDiffuse;
    if ( scatteredPower < NO_SCATTERING ) {
        return;
    }

    double diffuseDensity;
    if ( localDiffuse > 0 ) {
        diffuseDensity = cosTheta / M_PI;
    } else {
        diffuseDensity = 0.0;
    }

    double nonDiffuseDensity = 0.0;
    if ( localNonDiffuse > 0 ) {
        bool totalInternalReflection;
        // With the normal turned around, the two sides swap
        const Vector3Df idealDirection = cosIn >= 0
            ? ScatteringFrame::idealRefraction(inReversed, goodNormal, inIndex, outIndex, &totalInternalReflection)
            : ScatteringFrame::idealRefraction(inReversed, goodNormal, outIndex, inIndex, &totalInternalReflection);
        const double cosAlpha = dot(idealDirection, out);
        if ( cosAlpha > 0 ) {
            nonDiffuseDensity = (exponent + 1.0) * java::Math::pow(cosAlpha, static_cast<double>(exponent)) / (2.0 * M_PI);
        }
    }

    *density = (localDiffuse * diffuseDensity + localNonDiffuse * nonDiffuseDensity) / scatteredPower;
    *survival = scatteredPower;
}

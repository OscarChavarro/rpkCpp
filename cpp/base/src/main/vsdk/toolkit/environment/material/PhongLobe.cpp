#include "vsdk/toolkit/environment/material/PhongLobe.h"
#include "vsdk/toolkit/environment/material/ScatteringComponent.h"

const float PhongLobe::LOWEST_SPECULAR_EXPONENT = 250.0F;

PhongLobe::PhongLobe(const ColorRgb &inDiffuse, const ColorRgb &inNonDiffuse, const float inExponent):
    diffuse(inDiffuse),
    nonDiffuse(inNonDiffuse),
    exponent(inExponent)
{
}

PhongLobe::PhongLobe(const PhongLobe &other):
    diffuse(other.diffuse),
    nonDiffuse(other.nonDiffuse),
    exponent(other.exponent)
{
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

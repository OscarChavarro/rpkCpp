#include "vsdk/toolkit/scene/ConstantColorBackground.h"

ConstantColorBackground::ConstantColorBackground():
    color()
{
    color.clear();
}

ConstantColorBackground::ConstantColorBackground(const ColorRgb &backgroundColor):
    color(backgroundColor)
{
}

ColorRgb
ConstantColorBackground::radiance(
    Vector3D * /*position*/,
    Vector3D * /*direction*/,
    float *probabilityDensityFunction) const
{
    if ( probabilityDensityFunction != nullptr ) {
        *probabilityDensityFunction = ConstantColorBackground::INV_FOUR_PI;
    }
    return color;
}

Vector3D
ConstantColorBackground::sample(
    Vector3D * /*position*/,
    float xi1,
    float xi2,
    ColorRgb *radianceValue,
    float *probabilityDensityFunction) const
{
    const double phi = 2.0 * static_cast<double>(java::Math::PI) * static_cast<double>(xi1);
    const float z = 1.0F - 2.0F * xi2;
    const float radialSquared = java::Math::max(0.0F, 1.0F - z * z);
    const float radius = java::Math::sqrt(radialSquared);

    if ( radianceValue != nullptr ) {
        *radianceValue = color;
    }
    if ( probabilityDensityFunction != nullptr ) {
        *probabilityDensityFunction = ConstantColorBackground::INV_FOUR_PI;
    }

    Vector3D direction;
    direction.set(
        radius * static_cast<float>(java::Math::cos(phi)),
        radius * static_cast<float>(java::Math::sin(phi)),
        z);
    return direction;
}

ColorRgb
ConstantColorBackground::power(Vector3D * /*position*/) const {
    ColorRgb emittedPower;
    emittedPower.scaledCopy(ConstantColorBackground::FOUR_PI, color);
    return emittedPower;
}

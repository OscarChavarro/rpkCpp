#include <cmath>

#include "java/lang/Math.h"
#include "scene/ConstantColorBackground.h"

namespace {
static constexpr float FOUR_PI = 4.0f * static_cast<float>(M_PI);
static constexpr float INV_FOUR_PI = 1.0f / FOUR_PI;
}

ConstantColorBackground::ConstantColorBackground():
    color()
{
    color.clear();
}

ConstantColorBackground::ConstantColorBackground(const ColorRgb &backgroundColor):
    color(backgroundColor)
{
}

void
ConstantColorBackground::setColor(const ColorRgb &backgroundColor) {
    color = backgroundColor;
}

ColorRgb
ConstantColorBackground::getColor() const {
    return color;
}

ColorRgb
ConstantColorBackground::radiance(
    Vector3D * /*position*/,
    Vector3D * /*direction*/,
    float *probabilityDensityFunction) const
{
    if ( probabilityDensityFunction != nullptr ) {
        *probabilityDensityFunction = INV_FOUR_PI;
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
    const double phi = 2.0 * static_cast<double>(M_PI) * static_cast<double>(xi1);
    const float z = 1.0f - 2.0f * xi2;
    const float radialSquared = java::Math::max(0.0f, 1.0f - z * z);
    const float radius = std::sqrt(radialSquared);

    if ( radianceValue != nullptr ) {
        *radianceValue = color;
    }
    if ( probabilityDensityFunction != nullptr ) {
        *probabilityDensityFunction = INV_FOUR_PI;
    }

    Vector3D direction;
    direction.set(
        radius * static_cast<float>(std::cos(phi)),
        radius * static_cast<float>(std::sin(phi)),
        z);
    return direction;
}

ColorRgb
ConstantColorBackground::power(Vector3D * /*position*/) const {
    ColorRgb emittedPower;
    emittedPower.scaledCopy(FOUR_PI, color);
    return emittedPower;
}

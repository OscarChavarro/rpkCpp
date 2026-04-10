#include "scene/ConstantColorBackground.h"

const float ConstantColorBackground::FOUR_PI = ((float)12.56637061435917295385);
const float ConstantColorBackground::INV_FOUR_PI = ((float)0.07957747154594766788);

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
    if ( probabilityDensityFunction != NULL ) {
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
    const double phi = 2.0 * ((double)(PI)) * ((double)(xi1));
    const float z = 1.0f - 2.0f * xi2;
    const float radialSquared = Math::max(0.0f, 1.0f - z * z);
    const float radius = Math::sqrt(radialSquared);

    if ( radianceValue != NULL ) {
        *radianceValue = color;
    }
    if ( probabilityDensityFunction != NULL ) {
        *probabilityDensityFunction = ConstantColorBackground::INV_FOUR_PI;
    }

    Vector3D direction;
    direction.set(
        radius * ((float)(Math::cos(phi))),
        radius * ((float)(Math::sin(phi))),
        z);
    return direction;
}

ColorRgb
ConstantColorBackground::power(Vector3D * /*position*/) const {
    ColorRgb emittedPower;
    emittedPower.scaledCopy(ConstantColorBackground::FOUR_PI, color);
    return emittedPower;
}

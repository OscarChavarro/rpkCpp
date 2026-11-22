#include "java/lang/Math.h"
#include "vsdk/toolkit/scene/ConstantColorBackground.h"

const float ConstantColorBackground::FOUR_PI = ((float)12.56637061435917295385);
const float ConstantColorBackground::INV_FOUR_PI = ((float)0.07957747154594766788);

ConstantColorBackground::ConstantColorBackground():
    color()
{
    color.clear();
}

ConstantColorBackground::ConstantColorBackground(const ColorRgbMutable &backgroundColor):
    color(backgroundColor)
{
}

ColorRgbMutable
ConstantColorBackground::radiance(
    Vector3Df * /*position*/,
    Vector3Df * /*direction*/,
    float *probabilityDensityFunction) const
{
    if ( probabilityDensityFunction != NULL ) {
        *probabilityDensityFunction = ConstantColorBackground::INV_FOUR_PI;
    }
    return color;
}

Vector3Df
ConstantColorBackground::sample(
    Vector3Df * /*position*/,
    float xi1,
    float xi2,
    ColorRgbMutable *radianceValue,
    float *probabilityDensityFunction) const
{
    const double phi = 2.0 * ((double)(Math::PI)) * ((double)(xi1));
    const float z = 1.0f - 2.0f * xi2;
    const float radialSquared = Math::max(0.0f, 1.0f - z * z);
    const float radius = Math::sqrt(radialSquared);

    if ( radianceValue != NULL ) {
        *radianceValue = color;
    }
    if ( probabilityDensityFunction != NULL ) {
        *probabilityDensityFunction = ConstantColorBackground::INV_FOUR_PI;
    }

    Vector3Df direction;
    direction = Vector3Df(radius * ((float)(Math::cos(phi))), radius * ((float)(Math::sin(phi))), z);
    return direction;
}

ColorRgbMutable
ConstantColorBackground::power(Vector3Df * /*position*/) const {
    ColorRgbMutable emittedPower;
    emittedPower.scaledCopy(ConstantColorBackground::FOUR_PI, color);
    return emittedPower;
}

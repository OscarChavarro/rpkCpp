#include "java/lang/Math.h"
#include "vsdk/toolkit/scene/ConstantColorBackground.h"

const double ConstantColorBackground::FOUR_PI = ((double)12.56637061435917295385);
const double ConstantColorBackground::INV_FOUR_PI = ((double)0.07957747154594766788);

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
    Vector3Dd * /*position*/,
    Vector3Dd * /*direction*/,
    double *probabilityDensityFunction) const
{
    if ( probabilityDensityFunction != NULL ) {
        *probabilityDensityFunction = ConstantColorBackground::INV_FOUR_PI;
    }
    return color;
}

Vector3Dd
ConstantColorBackground::sample(
    Vector3Dd * /*position*/,
    double xi1,
    double xi2,
    ColorRgbMutable *radianceValue,
    double *probabilityDensityFunction) const
{
    const double phi = 2.0 * ((double)(Math::PI)) * ((double)(xi1));
    const double z = 1.0 - 2.0 * xi2;
    const double radialSquared = Math::max(0.0, 1.0 - z * z);
    const double radius = Math::sqrt(radialSquared);

    if ( radianceValue != NULL ) {
        *radianceValue = color;
    }
    if ( probabilityDensityFunction != NULL ) {
        *probabilityDensityFunction = ConstantColorBackground::INV_FOUR_PI;
    }

    Vector3Dd direction;
    direction = Vector3Dd(radius * ((double)(Math::cos(phi))), radius * ((double)(Math::sin(phi))), z);
    return direction;
}

ColorRgbMutable
ConstantColorBackground::power(Vector3Dd * /*position*/) const {
    ColorRgbMutable emittedPower;
    emittedPower.scaledCopy(ConstantColorBackground::FOUR_PI, color);
    return emittedPower;
}

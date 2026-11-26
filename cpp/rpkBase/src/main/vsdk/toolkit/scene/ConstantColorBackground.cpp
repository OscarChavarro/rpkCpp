#include "java/lang/Math.h"
#include "vsdk/toolkit/scene/ConstantColorBackground.h"

ConstantColorBackground::ConstantColorBackground():
    color()
{
}

ConstantColorBackground::ConstantColorBackground(const ColorRgb &backgroundColor):
    color(backgroundColor)
{
}

ColorRgb
ConstantColorBackground::radiance(
    Vector3Dd * /*position*/,
    Vector3Dd * /*direction*/,
    double *probabilityDensityFunction) const
{
    if ( probabilityDensityFunction != nullptr ) {
        *probabilityDensityFunction = ConstantColorBackground::INV_FOUR_PI;
    }
    return color;
}

Vector3Dd
ConstantColorBackground::sample(
    Vector3Dd * /*position*/,
    double xi1,
    double xi2,
    ColorRgb *radianceValue,
    double *probabilityDensityFunction) const
{
    const double phi = 2.0 * static_cast<double>(java::Math::PI) * static_cast<double>(xi1);
    const double z = 1.0 - 2.0 * xi2;
    const double radialSquared = java::Math::max(0.0, 1.0 - z * z);
    const double radius = java::Math::sqrt(radialSquared);

    if ( radianceValue != nullptr ) {
        *radianceValue = color;
    }
    if ( probabilityDensityFunction != nullptr ) {
        *probabilityDensityFunction = ConstantColorBackground::INV_FOUR_PI;
    }

    Vector3Dd direction;
    direction = Vector3Dd(radius * static_cast<double>(java::Math::cos(phi)), radius * static_cast<double>(java::Math::sin(phi)), z);
    return direction;
}

ColorRgb
ConstantColorBackground::power(Vector3Dd * /*position*/) const {
    return ColorRgb(ConstantColorBackground::FOUR_PI * color.getR(),
                    ConstantColorBackground::FOUR_PI * color.getG(),
                    ConstantColorBackground::FOUR_PI * color.getB());
}

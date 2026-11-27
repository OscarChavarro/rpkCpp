#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __CONSTANT_COLOR_BACKGROUND__
#define __CONSTANT_COLOR_BACKGROUND__

#include "vsdk/toolkit/scene/Background.h"

class ConstantColorBackground : public Background {
  public:
    ConstantColorBackground();
    explicit ConstantColorBackground(const ColorRgbMutable &backgroundColor);

    ColorRgbMutable
    radiance(Vector3Dd *position, Vector3Dd *direction, double *probabilityDensityFunction) const;

    Vector3Dd
    sample(
        Vector3Dd *position,
        double xi1,
        double xi2,
        ColorRgbMutable *radiance,
        double *probabilityDensityFunction) const;

    ColorRgbMutable
    power(Vector3Dd *position) const;

  private:
    static const double FOUR_PI;
    static const double INV_FOUR_PI;

    ColorRgbMutable color;
};

#endif

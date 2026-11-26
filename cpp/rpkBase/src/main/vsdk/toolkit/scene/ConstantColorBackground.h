#ifndef __CONSTANT_COLOR_BACKGROUND__
#define __CONSTANT_COLOR_BACKGROUND__

#include "vsdk/toolkit/scene/Background.h"

class ConstantColorBackground : public Background {
  public:
    ConstantColorBackground();
    explicit ConstantColorBackground(const ColorRgb &backgroundColor);

    ColorRgb
    radiance(Vector3Dd *position, Vector3Dd *direction, double *probabilityDensityFunction) const override;

    Vector3Dd
    sample(
        Vector3Dd *position,
        double xi1,
        double xi2,
        ColorRgb *radiance,
        double *probabilityDensityFunction) const override;

    ColorRgb
    power(Vector3Dd *position) const override;

  private:
    static constexpr double FOUR_PI = 12.56637061435917295385;
    static constexpr double INV_FOUR_PI = 0.07957747154594766788;

    ColorRgb color;
};

#endif

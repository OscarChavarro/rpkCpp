#ifndef CONSTANT_COLOR_BACKGROUND__
#define CONSTANT_COLOR_BACKGROUND__

#include "vsdk/toolkit/scene/Background.h"

class ConstantColorBackground : public Background {
  public:
    ConstantColorBackground();
    explicit ConstantColorBackground(const ColorRgbMutable &backgroundColor);

    ColorRgbMutable
    radiance(Vector3Df *position, Vector3Df *direction, float *probabilityDensityFunction) const override;

    Vector3Df
    sample(
        Vector3Df *position,
        float xi1,
        float xi2,
        ColorRgbMutable *radiance,
        float *probabilityDensityFunction) const override;

    ColorRgbMutable
    power(Vector3Df *position) const override;

  private:
    static constexpr float FOUR_PI = 12.56637061435917295385F;
    static constexpr float INV_FOUR_PI = 0.07957747154594766788F;

    ColorRgbMutable color;
};

#endif

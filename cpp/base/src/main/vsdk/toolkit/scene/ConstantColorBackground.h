#ifndef CONSTANT_COLOR_BACKGROUND__
#define CONSTANT_COLOR_BACKGROUND__

#include "vsdk/toolkit/scene/Background.h"

class ConstantColorBackground : public Background {
  public:
    ConstantColorBackground();
    explicit ConstantColorBackground(const ColorRgbMutable &backgroundColor);

    ColorRgbMutable
    radiance(Vector3D *position, Vector3D *direction, float *probabilityDensityFunction) const override;

    Vector3D
    sample(
        Vector3D *position,
        float xi1,
        float xi2,
        ColorRgbMutable *radiance,
        float *probabilityDensityFunction) const override;

    ColorRgbMutable
    power(Vector3D *position) const override;

  private:
    static constexpr float FOUR_PI = 12.56637061435917295385F;
    static constexpr float INV_FOUR_PI = 0.07957747154594766788F;

    ColorRgbMutable color;
};

#endif

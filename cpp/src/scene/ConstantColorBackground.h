#ifndef __CONSTANT_COLOR_BACKGROUND__
#define __CONSTANT_COLOR_BACKGROUND__

#include "scene/Background.h"

class ConstantColorBackground : public Background {
  public:
    ConstantColorBackground();
    explicit ConstantColorBackground(const ColorRgb &backgroundColor);

    ColorRgb
    radiance(Vector3D *position, Vector3D *direction, float *probabilityDensityFunction) const override;

    Vector3D
    sample(
        Vector3D *position,
        float xi1,
        float xi2,
        ColorRgb *radiance,
        float *probabilityDensityFunction) const override;

    ColorRgb
    power(Vector3D *position) const override;

  private:
    static constexpr float FOUR_PI = 12.56637061435917295385f;
    static constexpr float INV_FOUR_PI = 0.07957747154594766788f;

    ColorRgb color;
};

#endif

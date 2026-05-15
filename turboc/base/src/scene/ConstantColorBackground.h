#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __CONSTANT_COLOR_BACKGROUND__
#define __CONSTANT_COLOR_BACKGROUND__

#include "scene/Background.h"

class ConstantColorBackground : public Background {
  public:
    ConstantColorBackground();
    explicit ConstantColorBackground(const ColorRgb &backgroundColor);

    ColorRgb
    radiance(Vector3D *position, Vector3D *direction, float *probabilityDensityFunction) const;

    Vector3D
    sample(
        Vector3D *position,
        float xi1,
        float xi2,
        ColorRgb *radiance,
        float *probabilityDensityFunction) const;

    ColorRgb
    power(Vector3D *position) const;

  private:
    static const float FOUR_PI;
    static const float INV_FOUR_PI;

    ColorRgb color;
};

#endif

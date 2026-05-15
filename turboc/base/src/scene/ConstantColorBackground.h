#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __CONSTANT_COLOR_BACKGROUND__
#define __CONSTANT_COLOR_BACKGROUND__

#include "scene/Background.h"

class ConstantColorBackground : public Background {
  public:
    ConstantColorBackground();
    explicit ConstantColorBackground(const ColorRgbMutable &backgroundColor);

    ColorRgbMutable
    radiance(Vector3D *position, Vector3D *direction, float *probabilityDensityFunction) const;

    Vector3D
    sample(
        Vector3D *position,
        float xi1,
        float xi2,
        ColorRgbMutable *radiance,
        float *probabilityDensityFunction) const;

    ColorRgbMutable
    power(Vector3D *position) const;

  private:
    static const float FOUR_PI;
    static const float INV_FOUR_PI;

    ColorRgbMutable color;
};

#endif

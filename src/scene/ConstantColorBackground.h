#ifndef __CONSTANT_COLOR_BACKGROUND__
#define __CONSTANT_COLOR_BACKGROUND__

#include "scene/Background.h"

class ConstantColorBackground : public Background {
  public:
    ConstantColorBackground();
    explicit ConstantColorBackground(const ColorRgb &backgroundColor);

    void
    setColor(const ColorRgb &backgroundColor);

    ColorRgb
    getColor() const;

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
    ColorRgb color;
};

#endif

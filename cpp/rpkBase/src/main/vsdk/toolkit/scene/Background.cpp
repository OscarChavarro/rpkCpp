#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/scene/Background.h"

Background::Background():
    bkgPatch(nullptr)
{
}

Background::~Background() {
}

ColorRgbMutable
Background::radiance(
    Vector3Df * /*position*/,
    Vector3Df * /*direction*/,
    float *probabilityDensityFunction) const
{
    if ( probabilityDensityFunction != nullptr ) {
        *probabilityDensityFunction = 0.0F;
    }
    ColorRgbMutable black(0.0, 0.0, 0.0);
    black = ColorRgbMutable(0.0F, 0.0F, 0.0F);
    return black;
}

Vector3Df
Background::sample(
    Vector3Df * /*position*/,
    float /*xi1*/,
    float /*xi2*/,
    ColorRgbMutable *radianceValue,
    float *probabilityDensityFunction) const
{
    if ( radianceValue != nullptr ) {
        *radianceValue = ColorRgbMutable(0.0F, 0.0F, 0.0F);
    }
    if ( probabilityDensityFunction != nullptr ) {
        *probabilityDensityFunction = 0.0F;
    }
    return Vector3Df();
}

ColorRgbMutable
Background::power(Vector3Df * /*position*/) const {
    ColorRgbMutable black(0.0, 0.0, 0.0);
    black = ColorRgbMutable(0.0F, 0.0F, 0.0F);
    return black;
}

ColorRgbMutable
Background::backgroundPower(Background *bkg, Vector3Df *position) {
    if ( !bkg ) {
        ColorRgbMutable black(0.0, 0.0, 0.0);
        black = ColorRgbMutable(0.0, 0.0, 0.0);
        return black;
    } else {
        return bkg->power(position);
    }
}

#ifdef RAYTRACING_ENABLED
ColorRgbMutable
Background::backgroundRadiance(
        Background *bkg,
        Vector3Df *position,
        Vector3Df *direction,
        float *probabilityDensityFunction) {
    if ( !bkg ) {
        ColorRgbMutable black(0.0, 0.0, 0.0);
        black = ColorRgbMutable(0.0, 0.0, 0.0);
        return black;
    } else {
        return bkg->radiance(position, direction, probabilityDensityFunction);
    }
}
#endif

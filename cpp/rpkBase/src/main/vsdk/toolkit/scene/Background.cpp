#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/scene/Background.h"

Background::Background():
    bkgPatch(nullptr)
{
}

Background::~Background() {
}

ColorRgb
Background::radiance(
    Vector3Df * /*position*/,
    Vector3Df * /*direction*/,
    float *probabilityDensityFunction) const
{
    if ( probabilityDensityFunction != nullptr ) {
        *probabilityDensityFunction = 0.0F;
    }
    return ColorRgb(0.0F, 0.0F, 0.0F);
}

Vector3Df
Background::sample(
    Vector3Df * /*position*/,
    float /*xi1*/,
    float /*xi2*/,
    ColorRgb *radianceValue,
    float *probabilityDensityFunction) const
{
    if ( radianceValue != nullptr ) {
        *radianceValue = ColorRgb(0.0F, 0.0F, 0.0F);
    }
    if ( probabilityDensityFunction != nullptr ) {
        *probabilityDensityFunction = 0.0F;
    }
    return Vector3Df();
}

ColorRgb
Background::power(Vector3Df * /*position*/) const {
    return ColorRgb(0.0F, 0.0F, 0.0F);
}

ColorRgb
Background::backgroundPower(Background *bkg, Vector3Df *position) {
    if ( !bkg ) {
        return ColorRgb(0.0, 0.0, 0.0);
    } else {
        return bkg->power(position);
    }
}

#ifdef RAYTRACING_ENABLED
ColorRgb
Background::backgroundRadiance(
        Background *bkg,
        Vector3Df *position,
        Vector3Df *direction,
        float *probabilityDensityFunction) {
    if ( !bkg ) {
        return ColorRgb(0.0, 0.0, 0.0);
    } else {
        return bkg->radiance(position, direction, probabilityDensityFunction);
    }
}
#endif

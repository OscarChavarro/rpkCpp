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
    Vector3D * /*position*/,
    Vector3D * /*direction*/,
    float *probabilityDensityFunction) const
{
    if ( probabilityDensityFunction != nullptr ) {
        *probabilityDensityFunction = 0.0F;
    }
    ColorRgb black;
    black.setMonochrome(0.0F);
    return black;
}

Vector3D
Background::sample(
    Vector3D * /*position*/,
    float /*xi1*/,
    float /*xi2*/,
    ColorRgb *radianceValue,
    float *probabilityDensityFunction) const
{
    if ( radianceValue != nullptr ) {
        radianceValue->setMonochrome(0.0F);
    }
    if ( probabilityDensityFunction != nullptr ) {
        *probabilityDensityFunction = 0.0F;
    }
    return Vector3D();
}

ColorRgb
Background::power(Vector3D * /*position*/) const {
    ColorRgb black;
    black.setMonochrome(0.0F);
    return black;
}

ColorRgb
Background::backgroundPower(Background *bkg, Vector3D *position) {
    if ( !bkg ) {
        ColorRgb black;
        black.setMonochrome(0.0);
        return black;
    } else {
        return bkg->power(position);
    }
}

#ifdef RAYTRACING_ENABLED
ColorRgb
Background::backgroundRadiance(
        Background *bkg,
        Vector3D *position,
        Vector3D *direction,
        float *probabilityDensityFunction) {
    if ( !bkg ) {
        ColorRgb black;
        black.setMonochrome(0.0);
        return black;
    } else {
        return bkg->radiance(position, direction, probabilityDensityFunction);
    }
}
#endif

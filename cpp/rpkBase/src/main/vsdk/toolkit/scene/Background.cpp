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
    Vector3Dd * /*position*/,
    Vector3Dd * /*direction*/,
    double *probabilityDensityFunction) const
{
    if ( probabilityDensityFunction != nullptr ) {
        *probabilityDensityFunction = 0.0;
    }
    return ColorRgb(0.0, 0.0, 0.0);
}

Vector3Dd
Background::sample(
    Vector3Dd * /*position*/,
    double /*xi1*/,
    double /*xi2*/,
    ColorRgb *radianceValue,
    double *probabilityDensityFunction) const
{
    if ( radianceValue != nullptr ) {
        *radianceValue = ColorRgb(0.0, 0.0, 0.0);
    }
    if ( probabilityDensityFunction != nullptr ) {
        *probabilityDensityFunction = 0.0;
    }
    return Vector3Dd();
}

ColorRgb
Background::power(Vector3Dd * /*position*/) const {
    return ColorRgb(0.0, 0.0, 0.0);
}

ColorRgb
Background::backgroundPower(Background *bkg, Vector3Dd *position) {
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
        Vector3Dd *position,
        Vector3Dd *direction,
        double *probabilityDensityFunction) {
    if ( !bkg ) {
        return ColorRgb(0.0, 0.0, 0.0);
    } else {
        return bkg->radiance(position, direction, probabilityDensityFunction);
    }
}
#endif

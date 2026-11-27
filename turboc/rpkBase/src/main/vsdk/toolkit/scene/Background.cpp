#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/scene/Background.h"

Background::Background():
    bkgPatch(NULL)
{
}

Background::~Background() {
}

ColorRgbMutable
Background::radiance(
    Vector3Dd * /*position*/,
    Vector3Dd * /*direction*/,
    double *probabilityDensityFunction) const
{
    if ( probabilityDensityFunction != NULL ) {
        *probabilityDensityFunction = 0.0;
    }
    return ColorRgbMutable(0.0, 0.0, 0.0);
}

Vector3Dd
Background::sample(
    Vector3Dd * /*position*/,
    double /*xi1*/,
    double /*xi2*/,
    ColorRgbMutable *radianceValue,
    double *probabilityDensityFunction) const
{
    if ( radianceValue != NULL ) {
        *radianceValue = ColorRgbMutable(0.0, 0.0, 0.0);
    }
    if ( probabilityDensityFunction != NULL ) {
        *probabilityDensityFunction = 0.0;
    }
    return Vector3Dd();
}

ColorRgbMutable
Background::power(Vector3Dd * /*position*/) const {
    return ColorRgbMutable(0.0, 0.0, 0.0);
}

ColorRgbMutable
Background::backgroundPower(Background *bkg, Vector3Dd *position) {
    if ( !bkg ) {
        return ColorRgbMutable(0.0, 0.0, 0.0);
    } else {
        return bkg->power(position);
    }
}

#ifdef RAYTRACING_ENABLED
ColorRgbMutable
Background::backgroundRadiance(
        Background *bkg,
        Vector3Dd *position,
        Vector3Dd *direction,
        double *probabilityDensityFunction) {
    if ( !bkg ) {
        return ColorRgbMutable(0.0, 0.0, 0.0);
    } else {
        return bkg->radiance(position, direction, probabilityDensityFunction);
    }
}
#endif

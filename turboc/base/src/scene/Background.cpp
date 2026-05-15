#include "material/RendererConfiguration.h"
#include "scene/Background.h"

Background::Background():
    bkgPatch(NULL)
{
}

Background::~Background() {
}

ColorRgbMutable
Background::radiance(
    Vector3D * /*position*/,
    Vector3D * /*direction*/,
    float *probabilityDensityFunction) const
{
    if ( probabilityDensityFunction != NULL ) {
        *probabilityDensityFunction = 0.0f;
    }
    return ColorRgbMutable(0.0f, 0.0f, 0.0f);
}

Vector3D
Background::sample(
    Vector3D * /*position*/,
    float /*xi1*/,
    float /*xi2*/,
    ColorRgbMutable *radianceValue,
    float *probabilityDensityFunction) const
{
    if ( radianceValue != NULL ) {
        *radianceValue = ColorRgbMutable(0.0f, 0.0f, 0.0f);
    }
    if ( probabilityDensityFunction != NULL ) {
        *probabilityDensityFunction = 0.0f;
    }
    return Vector3D();
}

ColorRgbMutable
Background::power(Vector3D * /*position*/) const {
    return ColorRgbMutable(0.0f, 0.0f, 0.0f);
}

ColorRgbMutable
Background::backgroundPower(Background *bkg, Vector3D *position) {
    if ( !bkg ) {
        return ColorRgbMutable(0.0f, 0.0f, 0.0f);
    } else {
        return bkg->power(position);
    }
}

#ifdef RAYTRACING_ENABLED
ColorRgbMutable
Background::backgroundRadiance(
        Background *bkg,
        Vector3D *position,
        Vector3D *direction,
        float *probabilityDensityFunction) {
    if ( !bkg ) {
        return ColorRgbMutable(0.0f, 0.0f, 0.0f);
    } else {
        return bkg->radiance(position, direction, probabilityDensityFunction);
    }
}
#endif

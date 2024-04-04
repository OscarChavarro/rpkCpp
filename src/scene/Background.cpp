#include "scene/Background.h"

ColorRgb
backgroundRadiance(
    Background *bkg,
    Vector3D *position,
    Vector3D *direction,
    float *probabilityDensityFunction) {
    if ( !bkg || !bkg->methods->Radiance ) {
        ColorRgb black;
        colorSetMonochrome(black, 0.0);
        return black;
    } else {
        return bkg->methods->Radiance(bkg->data, position, direction, probabilityDensityFunction);
    }
}

ColorRgb
backgroundPower(Background *bkg, Vector3D *position) {
    if ( !bkg || !bkg->methods->Power ) {
        ColorRgb black;
        colorSetMonochrome(black, 0.0);
        return black;
    } else {
        return bkg->methods->Power(bkg->data, position);
    }
}

#include "scene/Background.h"

COLOR
backgroundRadiance(
    Background *bkg,
    Vector3D *position,
    Vector3D *direction,
    float *probabilityDensityFunction) {
    if ( !bkg || !bkg->methods->Radiance ) {
        COLOR black;
        colorSetMonochrome(black, 0.0);
        return black;
    } else {
        return bkg->methods->Radiance(bkg->data, position, direction, probabilityDensityFunction);
    }
}

COLOR
backgroundPower(Background *bkg, Vector3D *position) {
    if ( !bkg || !bkg->methods->Power ) {
        COLOR black;
        colorSetMonochrome(black, 0.0);
        return black;
    } else {
        return bkg->methods->Power(bkg->data, position);
    }
}

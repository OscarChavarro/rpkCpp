#include "scene/Background.h"

COLOR
BackgroundRadiance(
    Background *bkg,
    Vector3D *position,
    Vector3D *direction,
    float *pdf) {
    if ( !bkg || !bkg->methods->Radiance ) {
        COLOR black;
        COLORSETMONOCHROME(black, 0.);
        return black;
    } else {
        return bkg->methods->Radiance(bkg->data, position, direction, pdf);
    }
}

COLOR
BackgroundPower(Background *bkg, Vector3D *position) {
    if ( !bkg || !bkg->methods->Power ) {
        COLOR black;
        COLORSETMONOCHROME(black, 0.);
        return black;
    } else {
        return bkg->methods->Power(bkg->data, position);
    }
}

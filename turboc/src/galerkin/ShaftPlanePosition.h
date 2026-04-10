#ifndef __SHAFT_PLANE_POSITION__
#define __SHAFT_PLANE_POSITION__

#include "common/VSDK.h"

/**
Positions of item with respect to a plane or a shaft
*/
enum ShaftPlanePosition {
    INSIDE,
    OVERLAP,
    OUTSIDE,
    COPLANAR
};

#endif

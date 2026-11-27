#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
Vector3Dd Solid::doCenterOfMass() {
    return Vector3Dd(0, 0, 0);
}

const Class Solid::CLASS = { "Solid", &Volume::CLASS };

const Class *
Solid::getClass() const
{
    return &CLASS;
}

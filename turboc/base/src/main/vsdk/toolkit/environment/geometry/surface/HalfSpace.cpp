#include "vsdk/toolkit/environment/geometry/surface/HalfSpace.h"

const Class HalfSpace::CLASS = { "HalfSpace", &Surface::CLASS };

const Class *
HalfSpace::getClass() const
{
    return &CLASS;
}

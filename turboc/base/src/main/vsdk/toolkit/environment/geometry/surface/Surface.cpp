#include "vsdk/toolkit/environment/geometry/surface/Surface.h"

const Class Surface::CLASS = { "Surface", &Geometry::CLASS };

const Class *
Surface::getClass() const
{
    return &CLASS;
}

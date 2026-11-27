#include <stddef.h>
#include "vsdk/toolkit/environment/geometry/volume/Volume.h"
PolyhedralBoundedSolid* Volume::exportToPolyhedralBoundedSolid() { return NULL; }

const Class Volume::CLASS = { "Volume", &Geometry::CLASS };

const Class *
Volume::getClass() const
{
    return &CLASS;
}

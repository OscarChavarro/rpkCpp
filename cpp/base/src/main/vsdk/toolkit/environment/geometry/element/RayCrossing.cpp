#include "vsdk/toolkit/environment/geometry/element/RayCrossing.h"
RayCrossing::RayCrossing(double t, const Geometry* geometry, Kind kind)
    : RayBoundary(t, geometry), kind(kind), body(nullptr), material(nullptr) {}

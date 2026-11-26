#ifndef __RAY_CROSSING__
#define __RAY_CROSSING__

#include "vsdk/toolkit/environment/geometry/element/RayCrossingKind.h"
#include "vsdk/toolkit/environment/geometry/element/RayBoundary.h"

class Geometry;
class SceneSolid;
class Material;

// Leaf identity and optional instance/material attribution; pointers are borrowed.
class RayCrossing : public RayBoundary {
public:
    using Kind = RayCrossingKind;
    Kind kind;
    const SceneSolid* body;
    Material* material;

    RayCrossing(double t, const Geometry* geometry, Kind kind);
};

#endif

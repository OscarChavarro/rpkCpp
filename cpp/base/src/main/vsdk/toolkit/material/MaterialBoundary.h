#ifndef MATERIAL_BOUNDARY__
#define MATERIAL_BOUNDARY__

#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"

/**
Conversions between RenderPark's single precision Vector3D and VITRAL's
Vector3Df at the material boundary: the same three floats, so a value
crosses unchanged.
*/
class MaterialBoundary final {
  public:
    static Vector3Df toVitral(const Vector3D &v) {
        return Vector3Df(v.x, v.y, v.z);
    }

    static Vector3D fromVitral(const Vector3Df &v) {
        return Vector3D(v.x(), v.y(), v.z());
    }
};

#endif

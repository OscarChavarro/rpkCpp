#ifndef __SHADING_CONTEXT__
#define __SHADING_CONTEXT__

#include "common/linealAlgebra/CoordinateSystem.h"
#include "common/linealAlgebra/Vector2Dd.h"
#include "common/linealAlgebra/Vector3D.h"
#include "environment/geometry/elements/RayHitFlag.h"

class Material;

/**
Immutable shading input data used by material evaluation.
*/
class ShadingContext {
  private:
    Vector3D point;
    Vector3D geometricNormal;
    Vector3D shadingNormal;
    Vector3D texCoord;
    Vector2Dd uv;
    CoordinateSystem shadingFrame;
    Material *material;
    unsigned int flags;

  public:
    ShadingContext(
        const Vector3D &inPoint,
        const Vector3D &inGeometricNormal,
        const Vector3D &inShadingNormal,
        const Vector3D &inTexCoord,
        const Vector2Dd &inUv,
        const CoordinateSystem &inShadingFrame,
        Material *inMaterial,
        unsigned int inFlags):
        point(inPoint),
        geometricNormal(inGeometricNormal),
        shadingNormal(inShadingNormal),
        texCoord(inTexCoord),
        uv(inUv),
        shadingFrame(inShadingFrame),
        material(inMaterial),
        flags(inFlags)
    {}

    const Vector3D &getPoint() const { return point; }
    const Vector3D &getGeometricNormal() const { return geometricNormal; }
    const Vector3D &getShadingNormal() const { return shadingNormal; }
    const Vector3D &getTexCoord() const { return texCoord; }
    const Vector2Dd &getUv() const { return uv; }
    const CoordinateSystem &getShadingFrame() const { return shadingFrame; }
    Material *getMaterial() const { return material; }
    unsigned int getFlags() const { return flags; }
    bool hasFlag(unsigned int mask) const { return (flags & mask) == mask; }
};

#endif

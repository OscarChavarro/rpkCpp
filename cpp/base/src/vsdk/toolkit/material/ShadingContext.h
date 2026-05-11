#ifndef SHADING_CONTEXT__
#define SHADING_CONTEXT__

#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/material/ShadingContextFlag.h"

class Material;

/**
Immutable shading input data used by material evaluation.
Phase 1: introduced but not yet wired into BSDF/EDF APIs.
*/
class ShadingContext final {
  private:
    const Vector3D point;
    const Vector3D geometricNormal;
    const Vector3D shadingNormal;
    const Vector3D texCoord;
    const Vector2Dd uv;
    const CoordinateSystem shadingFrame;
    const Material * const material;
    const unsigned int flags;

  public:
    ShadingContext(
        const Vector3D &inPoint,
        const Vector3D &inGeometricNormal,
        const Vector3D &inShadingNormal,
        const Vector3D &inTexCoord,
        const Vector2Dd &inUv,
        const CoordinateSystem &inShadingFrame,
        const Material *inMaterial,
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
    const Material *getMaterial() const { return material; }
    unsigned int getFlags() const { return flags; }
    bool hasFlag(unsigned int mask) const { return (flags & mask) == mask; }
};

#endif

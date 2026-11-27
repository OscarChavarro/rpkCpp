#ifndef __SHADING_CONTEXT__
#define __SHADING_CONTEXT__

#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/material/ShadingContextFlag.h"

class RpkMaterial;

/**
Immutable shading input data used by material evaluation.
Phase 1: introduced but not yet wired into BSDF/EDF APIs.
*/
class ShadingContext {
  private:
    const Vector3Dd point;
    const Vector3Dd geometricNormal;
    const Vector3Dd shadingNormal;
    const Vector3Dd texCoord;
    const Vector2Dd uv;
    const CoordinateSystem shadingFrame;
    const RpkMaterial * const material;
    const unsigned int flags;

  public:
    ShadingContext(
        const Vector3Dd &inPoint,
        const Vector3Dd &inGeometricNormal,
        const Vector3Dd &inShadingNormal,
        const Vector3Dd &inTexCoord,
        const Vector2Dd &inUv,
        const CoordinateSystem &inShadingFrame,
        const RpkMaterial *inMaterial,
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

    const Vector3Dd &getPoint() const { return point; }
    const Vector3Dd &getGeometricNormal() const { return geometricNormal; }
    const Vector3Dd &getShadingNormal() const { return shadingNormal; }
    const Vector3Dd &getTexCoord() const { return texCoord; }
    const Vector2Dd &getUv() const { return uv; }
    const CoordinateSystem &getShadingFrame() const { return shadingFrame; }
    const RpkMaterial *getMaterial() const { return material; }
    unsigned int getFlags() const { return flags; }
    bool hasFlag(unsigned int mask) const { return (flags & mask) == mask; }
};

#endif

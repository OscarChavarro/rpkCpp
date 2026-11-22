#ifndef SHADING_CONTEXT__
#define SHADING_CONTEXT__

#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/material/ShadingContextFlag.h"

class Material;

/**
Immutable shading input data used by material evaluation.
Phase 1: introduced but not yet wired into BSDF/EDF APIs.
*/
class ShadingContext {
  private:
    const Vector3Df point;
    const Vector3Df geometricNormal;
    const Vector3Df shadingNormal;
    const Vector3Df texCoord;
    const Vector2Dd uv;
    const CoordinateSystem shadingFrame;
    const Material * const material;
    const unsigned int flags;

  public:
    ShadingContext(
        const Vector3Df &inPoint,
        const Vector3Df &inGeometricNormal,
        const Vector3Df &inShadingNormal,
        const Vector3Df &inTexCoord,
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

    const Vector3Df &getPoint() const { return point; }
    const Vector3Df &getGeometricNormal() const { return geometricNormal; }
    const Vector3Df &getShadingNormal() const { return shadingNormal; }
    const Vector3Df &getTexCoord() const { return texCoord; }
    const Vector2Dd &getUv() const { return uv; }
    const CoordinateSystem &getShadingFrame() const { return shadingFrame; }
    const Material *getMaterial() const { return material; }
    unsigned int getFlags() const { return flags; }
    bool hasFlag(unsigned int mask) const { return (flags & mask) == mask; }
};

#endif

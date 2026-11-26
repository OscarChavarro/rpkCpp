#ifndef __SURFACE_SHADING_POINT__
#define __SURFACE_SHADING_POINT__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

/**
What the directional functions of PhongMaterial read about the surface point
being shaded: the shading normal and the texture coordinate, each with a flag
telling whether the caller could determine it. The caller finds the point,
its normal and its texture coordinate; the material only reads them.

Immutable value.
*/
class SurfaceShadingPoint {
  private:
    Vector3Dd shadingNormal;
    Vector3Dd textureCoordinate;
    bool shadingNormalKnown;
    bool textureCoordinateKnown;

  public:
    SurfaceShadingPoint(
        const Vector3Dd &inShadingNormal,
        bool inShadingNormalKnown,
        const Vector3Dd &inTextureCoordinate,
        bool inTextureCoordinateKnown) :
        shadingNormal(inShadingNormal),
        textureCoordinate(inTextureCoordinate),
        shadingNormalKnown(inShadingNormalKnown),
        textureCoordinateKnown(inTextureCoordinateKnown)
    {
    }

    /** Unit normal leaving the surface on the side the scattering is evaluated from */
    const Vector3Dd &getShadingNormal() const { return shadingNormal; }
    bool hasShadingNormal() const { return shadingNormalKnown; }

    /** Texture coordinate (x and y used) */
    const Vector3Dd &getTextureCoordinate() const { return textureCoordinate; }
    bool hasTextureCoordinate() const { return textureCoordinateKnown; }
};

#endif

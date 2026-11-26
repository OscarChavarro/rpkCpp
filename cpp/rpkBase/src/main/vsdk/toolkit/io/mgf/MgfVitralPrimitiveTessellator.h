#ifndef __MGF_VITRAL_PRIMITIVE_TESSELLATOR__
#define __MGF_VITRAL_PRIMITIVE_TESSELLATOR__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

/**
Handlers of the MGF surface primitives (`sph`, `cyl`, `cone`, `torus`,
`ring` and `prism`) that build each one as the matching VITRAL geometry
(`Sphere`, `Cone`, `Torus`, `Tube::createWasherLamina` and `Prism`),
export it to a `PolyhedralBoundedSolid` and turn its faces into MGF faces
(see `MgfBoundaryFaceEmitter`).

The tessellation follows the classic MGF one: 4 * n divisions around each
circle and 2 * n along the meridians of the sphere, for n quarter circle
divisions, quadrilaterals with triangle fans at the poles and apexes, open
cones and cylinders, and the vertex normals of `MgfSurfaceNormalModel`.
The local frame of each primitive is the one of the classic tessellation
(`MgfTessellationMath::mgfMakeAxes` on the axis of the primitive), so the
vertices are at the same angles.

VITRAL primitives have positive sizes: an MGF primitive with negative radii
(whose surface faces inwards) is built with the absolute values and its
faces are turned inside out.
*/
class MgfVitralPrimitiveTessellator final {
  public:
    static int handleSphere(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
    static int handleCylinder(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
    static int handleCone(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
    static int handleTorus(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
    static int handleRing(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
    static int handlePrism(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);

  private:
    static int circleDivisions(const ParseRuntimeContext *context);
    static int emitCone(const Vector3Dd &bottomCenter, double bottomRadius, const Vector3Dd &topCenter, double topRadius, ParseRuntimeContext *context);
};

#endif

/**
Hit record structure, returned by ray-object intersection routines and
used as a parameter for BSDF/EDF queries
*/

#ifndef __RPK_RAY_HIT__
#define __RPK_RAY_HIT__

#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/material/ShadingContext.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"

class Patch; // TODO: this is coupling RpkRayHit with skin level classes :(
class RpkMaterial;

/**
Information returned by ray-geometry or ray-discretization intersection
routines
*/
class RpkRayHit {
  private:
    Vector3Dd point; // Intersection point
    const Patch *patch; // Patch that was hit
    Vector3Dd texCoord; // Texture coordinate
    Vector3Dd geometricNormal;
    const RpkMaterial *material; // RpkMaterial of hit surface
    CoordinateSystem shadingFrame; // Shading frame (Z = shading normal: hit->shadingFrame.getZ() == hit->normal)
    Vector2Dd uv; // Bi-linear / barycentric parameters of hit
    unsigned int flags; // Flags indicating which of the above fields have been filled in

    bool computeUv(Vector2Dd *inUv);
    bool hitInitialised() const;
    bool pointShadingFrame(Vector3Dd *inX, Vector3Dd *inY, Vector3Dd *inZ);

public:
    RpkRayHit();

    bool
    init(
        const Patch *inPatch,
        const Vector3Dd *inPoint,
        const Vector3Dd *inGeometryNormal,
        const RpkMaterial *inMaterial);

    bool getTexCoord(Vector3Dd *outTexCoord);
    bool shadingNormal(Vector3Dd *inNormal);
    ShadingContext shadingContext(bool *ok) const;

    const Patch *getPatch() const;
    void setPatch(const Patch *inPatch);
    Vector3Dd getPoint() const;
    void setPoint(const Vector3Dd *position);
    void setGeometricNormal(const Vector3Dd *inNormal);
    void setMaterial(const RpkMaterial *inMaterial);
    Vector2Dd getUv() const;
    void setUv(const Vector2Dd *inUv);
    void setUv(const double inU, const double inV);
    unsigned int getFlags() const;
    void setFlags(unsigned int inFlags);
    Vector3Dd getGeometricNormal() const;
    bool setShadingFrame(CoordinateSystem *frame);
    Vector3Dd getNormal() const;
    void setNormal(const Vector3Dd *n);
    CoordinateSystem getShadingFrame() const;
    const RpkMaterial * getMaterial() const;
    void setShadingFrame(const Vector3Dd *inX, const Vector3Dd *inY, const Vector3Dd *inZ);

};

inline const Patch *
RpkRayHit::getPatch() const {
    return patch;
}

inline void
RpkRayHit::setPatch(const Patch *inPatch) {
    patch = inPatch;
}

inline Vector3Dd
RpkRayHit::getPoint() const {
    return point;
}

inline void
RpkRayHit::setPoint(const Vector3Dd *position) {
    point = *position;
}

inline void
RpkRayHit::setGeometricNormal(const Vector3Dd *inNormal) {
    geometricNormal = *inNormal;
}

inline void
RpkRayHit::setMaterial(const RpkMaterial *inMaterial) {
    material = inMaterial;
}

inline Vector2Dd
RpkRayHit::getUv() const {
    return uv;
}

/**
Sets the (u,v) parameters of the hit and marks them as known. Everything
derived from (u,v) and cached in the record, the texture coordinate and the
interpolated shading normal and frame, is invalidated, so that a record
reused for several samples on one patch evaluates each sample at its own
(u,v) and not at the first one.
*/
inline void
RpkRayHit::setUv(const double inU, const double inV) {
    uv.x = inU;
    uv.y = inV;
    flags = (flags | UV)
        & ~(TEXTURE_COORDINATE | NORMAL | SHADING_FRAME);
}

inline void
RpkRayHit::setUv(const Vector2Dd *inUv) {
    setUv(inUv->x, inUv->y);
}

inline unsigned int
RpkRayHit::getFlags() const {
    return flags;
}

inline void
RpkRayHit::setFlags(unsigned int inFlags) {
    flags = inFlags;
}

inline Vector3Dd
RpkRayHit::getGeometricNormal() const {
    return geometricNormal;
}

inline Vector3Dd
RpkRayHit::getNormal() const {
    return shadingFrame.getZ();
}

inline void
RpkRayHit::setNormal(const Vector3Dd *n) {
    shadingFrame.setZ(*n);
}

inline CoordinateSystem
RpkRayHit::getShadingFrame() const {
    return shadingFrame;
}

inline const RpkMaterial *
RpkRayHit::getMaterial() const {
    return material;
}

inline void
RpkRayHit::setShadingFrame(const Vector3Dd *inX, const Vector3Dd *inY, const Vector3Dd *inZ) {
    shadingFrame.setX(*inX);
    shadingFrame.setY(*inY);
    shadingFrame.setZ(*inZ);
}

#endif

/**
Hit record structure, returned by ray-object intersection routines and
used as a parameter for BSDF/EDF queries
*/

#ifndef RPK_RAY_HIT__
#define RPK_RAY_HIT__

#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/material/ShadingContext.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"

class Patch; // TODO: this is coupling RayHit with skin level classes :(
class RpkMaterial;

/**
Information returned by ray-geometry or ray-discretization intersection
routines
*/
class RpkRayHit {
  private:
    Vector3D point; // Intersection point
    const Patch *patch; // Patch that was hit
    Vector3D texCoord; // Texture coordinate
    Vector3D geometricNormal;
    const RpkMaterial *material; // Material of hit surface
    CoordinateSystem shadingFrame; // Shading frame (Z = shading normal: hit->shadingFrame.getZ() == hit->normal)
    Vector2Dd uv; // Bi-linear / barycentric parameters of hit
    unsigned int flags; // Flags indicating which of the above fields have been filled in

    bool computeUv(Vector2Dd *inUv);
    bool hitInitialised() const;
    bool pointShadingFrame(Vector3D *inX, Vector3D *inY, Vector3D *inZ);

public:
    RpkRayHit();

    bool
    init(
        const Patch *inPatch,
        const Vector3D *inPoint,
        const Vector3D *inGeometryNormal,
        const RpkMaterial *inMaterial);

    bool getTexCoord(Vector3D *outTexCoord);
    bool shadingNormal(Vector3D *inNormal);
    ShadingContext shadingContext(bool *ok) const;

    const Patch *getPatch() const;
    void setPatch(const Patch *inPatch);
    Vector3D getPoint() const;
    void setPoint(const Vector3D *position);
    void setGeometricNormal(const Vector3D *inNormal);
    void setMaterial(const RpkMaterial *inMaterial);
    Vector2Dd getUv() const;
    void setUv(const Vector2Dd *inUv);
    void setUv(const double inU, const double inV);
    unsigned int getFlags() const;
    void setFlags(unsigned int inFlags);
    Vector3D getGeometricNormal() const;
    bool setShadingFrame(CoordinateSystem *frame);
    Vector3D getNormal() const;
    void setNormal(const Vector3D *n);
    CoordinateSystem getShadingFrame() const;
    const RpkMaterial * getMaterial() const;
    void setShadingFrame(const Vector3D *inX, const Vector3D *inY, const Vector3D *inZ);

};

inline const Patch *
RpkRayHit::getPatch() const {
    return patch;
}

inline void
RpkRayHit::setPatch(const Patch *inPatch) {
    patch = inPatch;
}

inline Vector3D
RpkRayHit::getPoint() const {
    return point;
}

inline void
RpkRayHit::setPoint(const Vector3D *position) {
    point = *position;
}

inline void
RpkRayHit::setGeometricNormal(const Vector3D *inNormal) {
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
    flags = (flags | RayHitFlag::UV)
        & ~(RayHitFlag::TEXTURE_COORDINATE | RayHitFlag::NORMAL | RayHitFlag::SHADING_FRAME);
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

inline Vector3D
RpkRayHit::getGeometricNormal() const {
    return geometricNormal;
}

inline Vector3D
RpkRayHit::getNormal() const {
    return shadingFrame.getZ();
}

inline void
RpkRayHit::setNormal(const Vector3D *n) {
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
RpkRayHit::setShadingFrame(const Vector3D *inX, const Vector3D *inY, const Vector3D *inZ) {
    shadingFrame.setX(*inX);
    shadingFrame.setY(*inY);
    shadingFrame.setZ(*inZ);
}

#endif

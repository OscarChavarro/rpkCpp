/**
Hit record structure, returned by ray-object intersection routines and
used as a parameter for BSDF/EDF queries
*/

#ifndef __RAY_HIT__
#define __RAY_HIT__

#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/material/ShadingContext.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"

class Patch; // TODO: this is coupling RayHit with skin level classes :(
class Material;

/**
Information returned by ray-geometry or ray-discretization intersection
routines
*/
class RayHit {
  private:
    Vector3Df point; // Intersection point
    Patch *patch; // Patch that was hit
    Vector3Df texCoord; // Texture coordinate
    Vector3Df geometricNormal;
    const Material *material; // Material of hit surface
    CoordinateSystem shadingFrame; // Shading frame (Z = shading normal: hit->shadingFrame.getZ() == hit->normal)
    Vector2Dd uv; // Bi-linear / barycentric parameters of hit
    unsigned int flags; // Flags indicating which of the above fields have been filled in

    bool computeUv(Vector2Dd *inUv);
    bool hitInitialised() const;
    bool pointShadingFrame(Vector3Df *inX, Vector3Df *inY, Vector3Df *inZ);

public:
    RayHit();

    bool
    init(
        Patch *inPatch,
        const Vector3Df *inPoint,
        const Vector3Df *inGeometryNormal,
        const Material *inMaterial);

    bool getTexCoord(Vector3Df *outTexCoord);
    bool shadingNormal(Vector3Df *inNormal);
    ShadingContext shadingContext(bool *ok) const;

    Patch *getPatch() const;
    void setPatch(Patch *inPatch);
    Vector3Df getPoint() const;
    void setPoint(const Vector3Df *position);
    void setGeometricNormal(const Vector3Df *inNormal);
    void setMaterial(const Material *inMaterial);
    Vector2Dd getUv() const;
    void setUv(const Vector2Dd *inUv);
    void setUv(const double inU, const double inV);
    unsigned int getFlags() const;
    void setFlags(unsigned int inFlags);
    Vector3Df getGeometricNormal() const;
    bool setShadingFrame(CoordinateSystem *frame);
    Vector3Df getNormal() const;
    void setNormal(const Vector3Df *n);
    CoordinateSystem getShadingFrame() const;
    const Material * getMaterial() const;
    void setShadingFrame(const Vector3Df *inX, const Vector3Df *inY, const Vector3Df *inZ);

};

inline Patch*
RayHit::getPatch() const {
    return patch;
}

inline void
RayHit::setPatch(Patch *inPatch) {
    patch = inPatch;
}

inline Vector3Df
RayHit::getPoint() const {
    return point;
}

inline void
RayHit::setPoint(const Vector3Df *position) {
    point = *position;
}

inline void
RayHit::setGeometricNormal(const Vector3Df *inNormal) {
    geometricNormal = *inNormal;
}

inline void
RayHit::setMaterial(const Material *inMaterial) {
    material = inMaterial;
}

inline Vector2Dd
RayHit::getUv() const {
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
RayHit::setUv(const double inU, const double inV) {
    uv.x = inU;
    uv.y = inV;
    flags = (flags | UV)
        & ~(TEXTURE_COORDINATE | NORMAL | SHADING_FRAME);
}

inline void
RayHit::setUv(const Vector2Dd *inUv) {
    setUv(inUv->x, inUv->y);
}

inline unsigned int
RayHit::getFlags() const {
    return flags;
}

inline void
RayHit::setFlags(unsigned int inFlags) {
    flags = inFlags;
}

inline Vector3Df
RayHit::getGeometricNormal() const {
    return geometricNormal;
}

inline Vector3Df
RayHit::getNormal() const {
    return shadingFrame.getZ();
}

inline void
RayHit::setNormal(const Vector3Df *n) {
    shadingFrame.setZ(*n);
}

inline CoordinateSystem
RayHit::getShadingFrame() const {
    return shadingFrame;
}

inline const Material *
RayHit::getMaterial() const {
    return material;
}

inline void
RayHit::setShadingFrame(const Vector3Df *inX, const Vector3Df *inY, const Vector3Df *inZ) {
    shadingFrame.setX(*inX);
    shadingFrame.setY(*inY);
    shadingFrame.setZ(*inZ);
}

#endif

/**
Hit record structure, returned by ray-object intersection routines and
used as a parameter for BSDF/EDF queries
*/

#ifndef __RAY_HIT__
#define __RAY_HIT__

#include "common/linealAlgebra/CoordinateSystem.h"
#include "common/linealAlgebra/Vector2Dd.h"
#include "common/linealAlgebra/Vector3D.h"
#include "common/RenderOptions.h"
#include "material/ShadingContext.h"
#include "RayHitFlag.h"

class Patch; // TODO: this is coupling RayHit with skin level classes :(
class Material;

/**
Information returned by ray-geometry or ray-discretization intersection
routines
*/
class RayHit {
  private:
    Vector3D point; // Intersection point
    Patch *patch; // Patch that was hit
    Vector3D texCoord; // Texture coordinate
    Vector3D geometricNormal;
    const Material *material; // Material of hit surface
    CoordinateSystem shadingFrame; // Shading frame (Z = shading normal: hit->shadingFrame.getZ() == hit->normal)
    Vector2Dd uv; // Bi-linear / barycentric parameters of hit
    unsigned int flags; // Flags indicating which of the above fields have been filled in

    bool computeUv(Vector2Dd *inUv);
    bool hitInitialised() const;
    bool pointShadingFrame(Vector3D *inX, Vector3D *inY, Vector3D *inZ);

public:
    RayHit();

    bool
    init(
        Patch *inPatch,
        const Vector3D *inPoint,
        const Vector3D *inGeometryNormal,
        const Material *inMaterial);

    bool getTexCoord(Vector3D *outTexCoord);
    bool shadingNormal(Vector3D *inNormal);
    ShadingContext shadingContext(bool *ok) const;

    Patch *getPatch() const;
    void setPatch(Patch *inPatch);
    Vector3D getPoint() const;
    void setPoint(const Vector3D *position);
    void setGeometricNormal(const Vector3D *inNormal);
    void setMaterial(const Material *inMaterial);
    Vector2Dd getUv() const;
    void setUv(const Vector2Dd *inUv);
    void setUv(const double inU, const double inV);
    unsigned int getFlags() const;
    void setFlags(unsigned int inFlags);

#ifdef RAYTRACING_ENABLED
    bool setShadingFrame(CoordinateSystem *frame);
    Vector3D getNormal() const;
    void setNormal(const Vector3D *n);
    CoordinateSystem getShadingFrame() const;
    const Material * getMaterial() const;
    Vector3D getGeometricNormal() const;
    void setShadingFrame(const Vector3D *inX, const Vector3D *inY, const Vector3D *inZ);
#endif

};

inline Patch*
RayHit::getPatch() const {
    return patch;
}

inline void
RayHit::setPatch(Patch *inPatch) {
    patch = inPatch;
}

inline Vector3D
RayHit::getPoint() const {
    return point;
}

inline void
RayHit::setPoint(const Vector3D *position) {
    point = *position;
}

inline void
RayHit::setGeometricNormal(const Vector3D *inNormal) {
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

inline void
RayHit::setUv(const Vector2Dd *inUv) {
    uv = *inUv;
}

inline void
RayHit::setUv(const double inU, const double inV) {
    uv.u = inU;
    uv.v = inV;
}

inline unsigned int
RayHit::getFlags() const {
    return flags;
}

inline void
RayHit::setFlags(unsigned int inFlags) {
    flags = inFlags;
}

#ifdef RAYTRACING_ENABLED
inline Vector3D
RayHit::getNormal() const {
    return shadingFrame.getZ();
}

inline void
RayHit::setNormal(const Vector3D *n) {
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

inline Vector3D
RayHit::getGeometricNormal() const {
    return geometricNormal;
}

inline void
RayHit::setShadingFrame(const Vector3D *inX, const Vector3D *inY, const Vector3D *inZ) {
    shadingFrame.setX(*inX);
    shadingFrame.setY(*inY);
    shadingFrame.setZ(*inZ);
}
#endif

#endif

/**
Hit record structure, returned by ray-object intersection routines and
used as a parameter for BSDF/EDF queries
*/

#ifndef __RAY_HIT__
#define __RAY_HIT__

#include "common/RenderOptions.h"
#include "common/linealAlgebra/Vector2Dd.h"
#include "common/linealAlgebra/Vector3D.h"
#include "common/linealAlgebra/CoordinateSystem.h"
#include "material/RayHitFlag.h"

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
    Material *material; // Material of hit surface
    CoordinateSystem shadingFrame; // Shading frame (Z = shading normal: hit->shadingFrame.Z == hit->normal)
    Vector2Dd uv; // Bi-linear / barycentric parameters of hit
    unsigned int flags; // Flags indicating which of the above fields have been filled in

    bool computeUv(Vector2Dd *inUv);
    bool hitInitialised() const;
    bool pointShadingFrame(Vector3D *inX, Vector3D *inY, Vector3D *inZ);

public:
    RayHit();

    int
    init(
        Patch *inPatch,
        const Vector3D *inPoint,
        const Vector3D *inGeometryNormal,
        Material *inMaterial);

    int getTexCoord(Vector3D *outTexCoord);
    int shadingNormal(Vector3D *inNormal);

    inline Patch*
    getPatch() const {
        return patch;
    }

    inline void
    setPatch(Patch *inPatch) {
        patch = inPatch;
    }

    inline Vector3D
    getPoint() const {
        return point;
    }

    inline void
    setPoint(const Vector3D *position) {
        point = *position;
    }

    inline void
    setGeometricNormal(const Vector3D *inNormal) {
        geometricNormal = *inNormal;
    }

    inline void
    setMaterial(Material *inMaterial) {
        material = inMaterial;
    }

    inline Vector2Dd
    getUv() const {
        return uv;
    }

    inline void
    setUv(const Vector2Dd *inUv) {
        uv = *inUv;
    }

    inline void
    setUv(const double inU, const double inV) {
        uv.u = inU;
        uv.v = inV;
    }

    inline unsigned int
    getFlags() const {
        return flags;
    }

    inline void
    setFlags(unsigned int inFlags) {
        flags = inFlags;
    }

#ifdef RAYTRACING_ENABLED
    inline Vector3D getNormal() const {
        return shadingFrame.Z;
    }

    inline void setNormal(const Vector3D *n) {
        shadingFrame.Z = *n;
    }

    bool setShadingFrame(CoordinateSystem *frame);

    inline CoordinateSystem
    getShadingFrame() const {
        return shadingFrame;
    }

    inline Material *
    getMaterial() const {
        return material;
    }

    inline Vector3D
    getGeometricNormal() const {
        return geometricNormal;
    }

    inline void
    setShadingFrame(const Vector3D *inX, const Vector3D *inY, const Vector3D *inZ) {
        shadingFrame.X = *inX;
        shadingFrame.Y = *inY;
        shadingFrame.Z = *inZ;
    }
#endif

};

#endif

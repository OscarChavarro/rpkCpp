#include "vsdk/toolkit/environment/geometry/elements/RpkRayHit.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

RpkRayHit::RpkRayHit():
    point(),
    patch(),
    texCoord(),
    geometricNormal(),
    material(),
    shadingFrame(),
    uv(),
    flags()
{
}

/**
Checks whether or not the hit record is properly initialised, that
means that at least 'patch' or 'geometry' plus 'point', 'geometricNormal', 'material'
and 'distance' are initialised. Returns TRUE if the structure is properly
initialised and FALSE if not
*/
bool
RpkRayHit::hitInitialised() const {
    return ((flags & PATCH) || (flags & GEOMETRY))
        && (flags & POINT)
        && (flags & GEOMETRIC_NORMAL)
        && (flags & MATERIAL)
        && (flags & DISTANCE);
}

/**
Initialises a hit record. Either patch or geometry shall be non-null. Returns
TRUE if the structure is properly initialised and FALSE if not.
This routine can be used in order to construct BSDF queries at other positions
than hit positions returned by ray intersection routines
*/
bool
RpkRayHit::init(
    const Patch *inPatch,
    const Vector3Dd *inPoint,
    const Vector3Dd *inGeometryNormal,
    const RpkMaterial *inMaterial)
{
    flags = 0;
    patch = inPatch;
    if ( inPatch != NULL ) {
        flags |= PATCH;
    }
    if ( inPoint != NULL ) {
        point = *inPoint;
        flags |= POINT;
    }
    if ( inGeometryNormal != NULL ) {
        geometricNormal = *inGeometryNormal;
        flags |= GEOMETRIC_NORMAL;
    }
    material = inMaterial;
    flags |= MATERIAL;
    flags |= DISTANCE;
    Vector3Dd localNormal;
    localNormal = Vector3Dd(0, 0, 0);
    texCoord = localNormal;
    shadingFrame.setX(localNormal);
    shadingFrame.setY(localNormal);
    shadingFrame.setZ(localNormal);
    uv.x = 0.0;
    uv.y = 0.0;
    return hitInitialised();
}

/**
Fills in (u,v) parameters of hit point on the hit patch, computing it if not
computed before. Returns FALSE if the (u,v) parameters could not be determined
*/
bool
RpkRayHit::computeUv(Vector2Dd *inUv) {
    if ( flags & UV ) {
        *inUv = uv;
        return true;
    }

    if ((flags & PATCH) && (flags & POINT) ) {
        patch->uv(&point, &uv.x, &uv.y);
        *inUv = uv;
        flags |= UV;
        return true;
    }

    return false;
}

/**
Fills in/computes texture coordinates of hit point
*/
bool
RpkRayHit::getTexCoord(Vector3Dd *outTexCoord) {
    if ( flags & TEXTURE_COORDINATE ) {
        *outTexCoord = texCoord;
        return true;
    }

    if ( !computeUv(&uv) ) {
        return false;
    }

    if ( flags & PATCH ) {
        texCoord = patch->textureCoordAtUv(uv.x, uv.y);
        *outTexCoord = texCoord;
        flags |= TEXTURE_COORDINATE;
        return true;
    }

    return false;
}

/**
Fills in shading normal (Z axis of shading frame) only, avoiding computation
of shading X and Y axis if possible
*/
bool
RpkRayHit::shadingNormal(Vector3Dd *inNormal) {
    if ( flags & SHADING_FRAME || flags & NORMAL ) {
        *inNormal = shadingFrame.getZ();
        return true;
    }

    Vector3Dd localNormal = shadingFrame.getZ();
    if ( !pointShadingFrame(NULL, NULL, &localNormal) ) {
        return false;
    }

    flags |= NORMAL;
    shadingFrame.setZ(localNormal);
    *inNormal = shadingFrame.getZ();
    return true;
}

ShadingContext
RpkRayHit::shadingContext(bool *ok) const {
    Vector3Dd normal;
    Vector3Dd texCoord;
    unsigned int localFlags = 0;
    bool localOk = true;

    if ( !const_cast<RpkRayHit *>(this)->shadingNormal(&normal) ) {
        localOk = false;
        normal = Vector3Dd(0.0, 0.0, 1.0);
    } else {
        localFlags |= SHCTX_NORMAL;
    }

    if ( const_cast<RpkRayHit *>(this)->getTexCoord(&texCoord) ) {
        localFlags |= SHCTX_TEXTURE_COORDINATE;
    } else {
        texCoord = Vector3Dd(0.0, 0.0, 0.0);
    }

    if ( ok != NULL ) {
        *ok = localOk;
    }

    return ShadingContext(
        point,
        geometricNormal,
        normal,
        texCoord,
        uv,
        shadingFrame,
        material,
        localFlags);
}

/**
Computes shading frame at hit point. Z is the shading normal. Returns FALSE
if the shading frame could not be determined.
If X and Y are null pointers, only the shading normal is returned in Z
possibly avoiding computations of the X and Y axis
*/
bool
RpkRayHit::pointShadingFrame(Vector3Dd *inX, Vector3Dd *inY, Vector3Dd *inZ) {
    bool success = false;

    if ( !hitInitialised() ) {
        Logger::reportMessage("RayHit", Logger::WARNING, "pointShadingFrame", "uninitialised hit structure");
        return false;
    }

    if ( material && material->getBsdf() ) {
        ShadingContext context(
            getPoint(),
            getGeometricNormal(),
            shadingFrame.getZ(),
            texCoord,
            uv,
            shadingFrame,
            material,
            0);
        success = PhongBidirScattDistFunc::bsdfShadingFrame(context, inX, inY, inZ);
    }

    if ( !success && material != NULL && material->getEdf() != NULL ) {
        ShadingContext context(
            getPoint(),
            getGeometricNormal(),
            shadingFrame.getZ(),
            texCoord,
            uv,
            shadingFrame,
            material,
            0);
        success = PhongEmitDistFunc::edfShadingFrame(context, inX, inY, inZ);
    }

    if ( !success && computeUv(&uv) ) {
        // Make default shading frame
        patch->interpolatedFrameAtUv(uv.x, uv.y, inX, inY, inZ);
        success = true;
    }

    return success;
}

/**
Fills in shading frame: Z is the shading normal
*/
bool
RpkRayHit::setShadingFrame(CoordinateSystem *frame) {
    if ( flags & SHADING_FRAME ) {
        frame->setX(shadingFrame.getX());
        frame->setY(shadingFrame.getY());
        frame->setZ(shadingFrame.getZ());
        return true;
    }

    Vector3Dd shadingX = shadingFrame.getX();
    Vector3Dd shadingY = shadingFrame.getY();
    Vector3Dd shadingZ = shadingFrame.getZ();

    if ( !pointShadingFrame(&shadingX, &shadingY, &shadingZ) ) {
        return false;
    }

    shadingFrame.setX(shadingX);
    shadingFrame.setY(shadingY);
    shadingFrame.setZ(shadingZ);
    flags |= SHADING_FRAME | NORMAL;

    frame->setX(shadingFrame.getX());
    frame->setY(shadingFrame.getY());
    frame->setZ(shadingFrame.getZ());
    return true;
}

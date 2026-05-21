#include "environment/geometry/elements/RayHit.h"
#include "common/logging/Logger.h"
#include "material/RendererConfiguration.h"
#include "environment/geometry/elements/Patch.h"

RayHit::RayHit():
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
RayHit::hitInitialised() const {
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
RayHit::init(
    Patch *inPatch,
    const Vector3D *inPoint,
    const Vector3D *inGeometryNormal,
    const Material *inMaterial)
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
    Vector3D localNormal;
    localNormal.set(0, 0, 0);
    texCoord = localNormal;
    shadingFrame.setX(localNormal);
    shadingFrame.setY(localNormal);
    shadingFrame.setZ(localNormal);
    uv.u = 0.0;
    uv.v = 0.0;
    return hitInitialised();
}

/**
Fills in (u,v) parameters of hit point on the hit patch, computing it if not
computed before. Returns FALSE if the (u,v) parameters could not be determined
*/
bool
RayHit::computeUv(Vector2Dd *inUv) {
    if ( flags & UV ) {
        *inUv = uv;
        return true;
    }

    if ((flags & PATCH) && (flags & POINT) ) {
        patch->uv(&point, &uv.u, &uv.v);
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
RayHit::getTexCoord(Vector3D *outTexCoord) {
    if ( flags & TEXTURE_COORDINATE ) {
        *outTexCoord = texCoord;
        return true;
    }

    if ( !computeUv(&uv) ) {
        return false;
    }

    if ( flags & PATCH ) {
        texCoord = patch->textureCoordAtUv(uv.u, uv.v);
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
RayHit::shadingNormal(Vector3D *inNormal) {
    if ( flags & SHADING_FRAME || flags & NORMAL ) {
        *inNormal = shadingFrame.getZ();
        return true;
    }

    Vector3D localNormal = shadingFrame.getZ();
    if ( !pointShadingFrame(NULL, NULL, &localNormal) ) {
        return false;
    }

    flags |= NORMAL;
    shadingFrame.setZ(localNormal);
    *inNormal = shadingFrame.getZ();
    return true;
}

ShadingContext
RayHit::shadingContext(bool *ok) const {
    Vector3D normal;
    Vector3D texCoord;
    unsigned int localFlags = 0;
    bool localOk = true;

    if ( !const_cast<RayHit *>(this)->shadingNormal(&normal) ) {
        localOk = false;
        normal.set(0.0, 0.0, 1.0);
    } else {
        localFlags |= SHCTX_NORMAL;
    }

    if ( const_cast<RayHit *>(this)->getTexCoord(&texCoord) ) {
        localFlags |= SHCTX_TEXTURE_COORDINATE;
    } else {
        texCoord.set(0.0, 0.0, 0.0);
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
RayHit::pointShadingFrame(Vector3D *inX, Vector3D *inY, Vector3D *inZ) {
    bool success = false;

    if ( !hitInitialised() ) {
        Logger::warning("pointShadingFrame", "uninitialised hit structure");
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
        patch->interpolatedFrameAtUv(uv.u, uv.v, inX, inY, inZ);
        success = true;
    }

    return success;
}

#ifdef RAYTRACING_ENABLED
/**
Fills in shading frame: Z is the shading normal
*/
bool
RayHit::setShadingFrame(CoordinateSystem *frame) {
    if ( flags & SHADING_FRAME ) {
        frame->setX(shadingFrame.getX());
        frame->setY(shadingFrame.getY());
        frame->setZ(shadingFrame.getZ());
        return true;
    }

    Vector3D shadingX = shadingFrame.getX();
    Vector3D shadingY = shadingFrame.getY();
    Vector3D shadingZ = shadingFrame.getZ();

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
#endif

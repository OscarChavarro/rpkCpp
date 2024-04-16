#include "common/error.h"
#include "skin/Patch.h"
#include "material/RayHit.h"

RayHit::RayHit():
    patch(),
    point(),
    geometricNormal(),
    material(),
    normal(),
    texCoord(),
    X(),
    Y(),
    Z(),
    uv(),
    dist(),
    flags()
{
}

/**
Checks whether or not the hit record is properly initialised, that
means that at least 'patch' or 'geom' plus 'point', 'geometricNormal', 'material'
and 'dist' are initialised. Returns TRUE if the structure is properly
initialised and FALSE if not
*/
static int
hitInitialised(RayHit *hit) {
    return ((hit->flags & HIT_PATCH) || (hit->flags & HIT_GEOMETRY))
           && (hit->flags & HIT_POINT)
           && (hit->flags & HIT_GEOMETRIC_NORMAL)
           && (hit->flags & HIT_MATERIAL)
           && (hit->flags & HIT_DIST);
}

/**
Initialises a hit record. Either patch or geom shall be non-null. Returns
TRUE if the structure is properly initialised and FALSE if not.
This routine can be used in order to construct BSDF queries at other positions
than hit positions returned by ray intersection routines
*/
int
RayHit::init(
    Patch *inPatch,
    Geometry *inGeometry,
    Vector3D *inPoint,
    Vector3D *inGeometryNormal,
    Material *inMaterial,
    float inDistance)
{
    flags = 0;
    patch = inPatch;
    if ( inPatch != nullptr ) {
        flags |= HIT_PATCH;
    }
    if ( inGeometry != nullptr ) {
        flags |= HIT_GEOMETRY;
    }
    if ( inPoint != nullptr ) {
        point = *inPoint;
        flags |= HIT_POINT;
    }
    if ( inGeometryNormal != nullptr ) {
        geometricNormal = *inGeometryNormal;
        flags |= HIT_GEOMETRIC_NORMAL;
    }
    material = inMaterial;
    flags |= HIT_MATERIAL;
    dist = inDistance;
    flags |= HIT_DIST;
    normal.set(0, 0, 0);
    texCoord = normal;
    X = normal;
    Y = normal;
    Z = normal;
    uv.u = uv.v = 0.0;
    return hitInitialised(this);
}

/**
Fills in (u,v) parameters of hit point on the hit patch, computing it if not
computed before. Returns FALSE if the (u,v) parameters could not be determined
*/
int
RayHit::hitUv(Vector2Dd *inUv) {
    if ( flags & HIT_UV ) {
        *inUv = uv;
        return true;
    }

    if ( (flags & HIT_PATCH) && (flags & HIT_POINT) ) {
        patch->uv(&point, &uv.u, &uv.v);
        *inUv = uv;
        flags |= HIT_UV;
        return true;
    }

    return false;
}

/**
Fills in/computes texture coordinates of hit point
*/
int
RayHit::getTexCoord(Vector3D *outTexCoord) {
    if ( flags & HIT_TEXTURE_COORDINATE ) {
        *outTexCoord = texCoord;
        return true;
    }

    if ( !hitUv(&uv) ) {
        return false;
    }

    if ( flags & HIT_PATCH ) {
        texCoord = patch->textureCoordAtUv(uv.u, uv.v);
        *outTexCoord = texCoord;
        flags |= HIT_TEXTURE_COORDINATE;
        return true;
    }

    return false;
}

/**
Computes shading frame at hit point. Z is the shading normal. Returns FALSE
if the shading frame could not be determined.
If X and Y are null pointers, only the shading normal is returned in Z
possibly avoiding computations of the X and Y axis
*/
int
RayHit::hitPointShadingFrame(Vector3D *inX, Vector3D *inY, Vector3D *inZ) {
    int success = false;

    if ( !hitInitialised(this) ) {
        logWarning("hitPointShadingFrame", "uninitialised hit structure");
        return false;
    }

    if ( material && material->bsdf ) {
        success = bsdfShadingFrame(material->bsdf, this, inX, inY, inZ);
    }

    if ( !success && material && material->edf ) {
        success = edfShadingFrame(material->edf, this, inX, inY, inZ);
    }

    if ( !success && hitUv(&uv) ) {
        // Make default shading frame
        patch->interpolatedFrameAtUv(uv.u, uv.v, inX, inY, inZ);
        success = true;
    }

    return success;
}

/**
Fills in shading frame: Z is the shading normal
*/
int
RayHit::shadingFrame(Vector3D *inX, Vector3D *inY, Vector3D *inZ) {
    if ( flags & HIT_SHADING_FRAME ) {
        *inX = X;
        *inY = Y;
        *inZ = Z;
        return true;
    }

    if ( !hitPointShadingFrame(&X, &Y, &Z) ) {
        return false;
    }

    normal = Z; // shading normal
    flags |= HIT_SHADING_FRAME | HIT_NORMAL;

    *inX = X;
    *inY = Y;
    *inZ = Z;
    return true;
}

/**
Fills in shading normal (Z axis of shading frame) only, avoiding computation
of shading X and Y axis if possible
*/
int
RayHit::shadingNormal(Vector3D *inNormal) {
    if ( flags & HIT_SHADING_FRAME || flags & HIT_NORMAL ) {
        *inNormal = normal;
        return true;
    }

    if ( !hitPointShadingFrame(nullptr, nullptr, &normal) ) {
        return false;
    }

    flags |= HIT_NORMAL;
    *inNormal = Z = normal;
    return true;
}

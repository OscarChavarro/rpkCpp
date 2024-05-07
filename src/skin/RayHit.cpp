#include "common/error.h"
#include "skin/Patch.h"
#include "material/RayHit.h"

#define HIT_GEOMETRY 0x01 // Intersected Geometry
#define HIT_TEXTURE_COORDINATE 0x200 // Texture coordinates

RayHit::RayHit():
    texCoord(),
    patch(),
    point(),
    geometricNormal(),
    material(),
    normal(),
    X(),
    Y(),
    Z(),
    uv(),
    distance(),
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
    return ((flags & HIT_PATCH) || (flags & HIT_GEOMETRY))
        && (flags & HIT_POINT)
        && (flags & HIT_GEOMETRIC_NORMAL)
        && (flags & HIT_MATERIAL)
        && (flags & HIT_DIST);
}

/**
Initialises a hit record. Either patch or geometry shall be non-null. Returns
TRUE if the structure is properly initialised and FALSE if not.
This routine can be used in order to construct BSDF queries at other positions
than hit positions returned by ray intersection routines
*/
int
RayHit::init(
    Patch *inPatch,
    const Geometry *inGeometry,
    const Vector3D *inPoint,
    const Vector3D *inGeometryNormal,
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
    distance = inDistance;
    flags |= HIT_DIST;
    normal.set(0, 0, 0);
    texCoord = normal;
    X = normal;
    Y = normal;
    Z = normal;
    uv.u = 0.0;
    uv.v = 0.0;
    return hitInitialised();
}

/**
Fills in (u,v) parameters of hit point on the hit patch, computing it if not
computed before. Returns FALSE if the (u,v) parameters could not be determined
*/
int
RayHit::computeUv(Vector2Dd *inUv) {
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

    if ( !computeUv(&uv) ) {
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
RayHit::pointShadingFrame(Vector3D *inX, Vector3D *inY, Vector3D *inZ) {
    int success = false;

    if ( !hitInitialised() ) {
        logWarning("pointShadingFrame", "uninitialised hit structure");
        return false;
    }

    if ( material && material->getBsdf() ) {
        success = PhongBidirectionalScatteringDistributionFunction::bsdfShadingFrame(this, inX, inY, inZ);
    }

    if ( !success && material != nullptr && material->getEdf() != nullptr ) {
        success = PhongEmittanceDistributionFunction::edfShadingFrame(this, inX, inY, inZ);
    }

    if ( !success && computeUv(&uv) ) {
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

    if ( !pointShadingFrame(&X, &Y, &Z) ) {
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

    if ( !pointShadingFrame(nullptr, nullptr, &normal) ) {
        return false;
    }

    flags |= HIT_NORMAL;
    *inNormal = Z = normal;
    return true;
}

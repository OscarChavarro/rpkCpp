#include "material/Material.h"
#include "material/hit.h"
#include "skin/Patch.h"

int
HitInitialised(RayHit *hit) {
    return ((hit->flags & HIT_PATCH) || (hit->flags & HIT_GEOM))
           && (hit->flags & HIT_POINT)
           && (hit->flags & HIT_GNORMAL)
           && (hit->flags & HIT_MATERIAL)
           && (hit->flags & HIT_DIST);
}

int
InitHit(
        RayHit *hit,
        Patch *patch,
        Geometry *geom,
        Vector3D *point,
        Vector3D *gnormal,
        Material *material,
        float dist)
{
    hit->flags = 0;
    hit->patch = patch;
    if ( patch ) {
        hit->flags |= HIT_PATCH;
    }
    hit->geom = geom;
    if ( geom ) {
        hit->flags |= HIT_GEOM;
    }
    if ( point ) {
        hit->point = *point;
        hit->flags |= HIT_POINT;
    }
    if ( gnormal ) {
        hit->gnormal = *gnormal;
        hit->flags |= HIT_GNORMAL;
    }
    hit->material = material;
    hit->flags |= HIT_MATERIAL;
    hit->dist = dist;
    hit->flags |= HIT_DIST;
    VECTORSET(hit->normal, 0, 0, 0);
    hit->texCoord = hit->X = hit->Y = hit->Z = hit->normal;
    hit->uv.u = hit->uv.v = 0.;
    return HitInitialised(hit);
}

int
HitUV(RayHit *hit, Vector2Dd *uv) {
    if ( hit->flags & HIT_UV ) {
        *uv = hit->uv;
        return true;
    }

    if ((hit->flags & HIT_PATCH) && (hit->flags && HIT_POINT)) {
        patchUv(hit->patch, &hit->point, &hit->uv.u, &hit->uv.v);
        *uv = hit->uv;
        hit->flags |= HIT_UV;
        return true;
    }

    return false;
}

int
HitTexCoord(RayHit *hit, Vector3D *texCoord) {
    if ( hit->flags & HIT_TEXCOORD ) {
        *texCoord = hit->texCoord;
        return true;
    }

    if ( !HitUV(hit, &hit->uv)) {
        return false;
    }

    if ( hit->flags & HIT_PATCH ) {
        *texCoord = hit->texCoord = patchTextureCoordAtUv(hit->patch, hit->uv.u, hit->uv.v);
        hit->flags |= HIT_TEXCOORD;
        return true;
    }

    return false;
}

int
HitShadingFrame(RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z) {
    if ( hit->flags & HIT_SHADINGFRAME ) {
        *X = hit->X;
        *Y = hit->Y;
        *Z = hit->Z;
        return true;
    }

    if ( !materialShadingFrame(hit, &hit->X, &hit->Y, &hit->Z)) {
        return false;
    }

    hit->normal = hit->Z; // shading normal
    hit->flags |= HIT_SHADINGFRAME | HIT_NORMAL;

    *X = hit->X;
    *Y = hit->Y;
    *Z = hit->Z;
    return true;
}

int
HitShadingNormal(RayHit *hit, Vector3D *normal) {
    if ( hit->flags & HIT_SHADINGFRAME || hit->flags & HIT_NORMAL ) {
        *normal = hit->normal;
        return true;
    }

    if ( !materialShadingFrame(hit, nullptr, nullptr, &hit->normal)) {
        return false;
    }

    hit->flags |= HIT_NORMAL;
    *normal = hit->Z = hit->normal;
    return true;
}

#include <cstdlib>

#include "common/error.h"
#include "material/statistics.h"
#include "skin/geom.h"

GEOM *excludedGeom1 = (GEOM *) nullptr;
GEOM *excludedGeom2 = (GEOM *) nullptr;

static int currentMaxId = 0;

static void
BoundsEnlargeTinyBit(float *bounds) {
    float Dx = (float)((bounds[MAX_X] - bounds[MIN_X]) * 1e-4);
    float Dy = (float)((bounds[MAX_Y] - bounds[MIN_Y]) * 1e-4);
    float Dz = (float)((bounds[MAX_Z] - bounds[MIN_Z]) * 1e-4);
    if ( Dx < EPSILON ) {
        Dx = EPSILON;
    }
    if ( Dy < EPSILON ) {
        Dy = EPSILON;
    }
    if ( Dz < EPSILON ) {
        Dz = EPSILON;
    }
    bounds[MIN_X] -= Dx;
    bounds[MAX_X] += Dx;
    bounds[MIN_Y] -= Dy;
    bounds[MAX_Y] += Dy;
    bounds[MIN_Z] -= Dz;
    bounds[MAX_Z] += Dz;
}

/**
This function is used to create a new GEOMetry with given specific data and
methods. A pointer to the new GEOMetry is returned
*/
GEOM *
GeomCreate(void *obj, GEOM_METHODS *methods) {
    if ( obj == nullptr) {
        return (GEOM *) nullptr;
    }

    GEOM *p = (GEOM *)malloc(sizeof(GEOM));
    GLOBAL_statistics_numberOfGeometries++;
    p->id = currentMaxId++;
    p->obj = obj;
    p->methods = methods;

    if ( methods->bounds ) {
        methods->bounds(obj, p->bounds);
        /* enlarge bounding box a tiny bit for more conservative bounding box culling */
        BoundsEnlargeTinyBit(p->bounds);
        p->bounded = true;
    } else {
        BoundsInit(p->bounds);
        p->bounded = false;
    }
    p->shaftcullgeom = false;

    p->radiance_data = (void *) nullptr;
    p->tmp.i = 0;
    p->omit = false;

    p->dlistid = -1;

    return p;
}

/**
This function prints the GEOMetry data to the file out
*/
void
GeomPrint(FILE *out, GEOM *geom) {
    fprintf(out, "Geom %d, bounded = %s, shaftcullgeom = %s:\n",
            geom->id,
            geom->bounded ? "TRUE" : "FALSE",
            geom->shaftcullgeom ? "TRUE" : "FALSE");

    geom->methods->print(out, geom->obj);
}

/**
This function returns a bounding box for the GEOMetry
*/
float *
GeomBounds(GEOM *geom) {
    return geom->bounds;
}

/**
This function destroys the given GEOMetry
*/
void
GeomDestroy(GEOM *geom) {
    geom->methods->destroy(geom->obj);
    free(geom);
    GLOBAL_statistics_numberOfGeometries--;
}

/**
This function returns nonzero if the given GEOMetry is an aggregate. An
aggregate is a geometry that consists of simpler geometries. Currently,
there is only one type of aggregate geometry: the compound, which is basically
just a list of simpler geometries. Other aggregate geometries are also
possible, e.g. CSG objects. If the given GEOMetry is a primitive, zero is
returned. A primitive GEOMetry is a GEOMetry that does not consist of
simpler GEOMetries
*/
int
GeomIsAggregate(GEOM *geom) {
    return geom->methods->primlist != (GEOMLIST *(*)(void *)) nullptr;
}

/**
Returns a linear list of the simpler GEOMEtries making up an aggregate GEOMetry.
A nullptr pointer is returned if the GEOMetry is a primitive
*/
GEOMLIST *
GeomPrimList(GEOM *geom) {
    if ( geom->methods->primlist ) {
        return geom->methods->primlist(geom->obj);
    } else {
        return (GEOMLIST *) nullptr;
    }
}

/**
Returns a linear list of patches making up a primitive GEOMetry. A nullptr
pointer is returned if the given GEOMetry is an aggregate
*/
PATCHLIST *
GeomPatchList(GEOM *geom) {
    if ( geom->methods->patchlist ) {
        return geom->methods->patchlist(geom->obj);
    } else {
        return (PATCHLIST *) nullptr;
    }
}

/**
This routine creates and returns a duplicate of the given geometry. Needed for
shaft culling.
*/
GEOM *
GeomDuplicate(GEOM *geom) {
    if ( !geom->methods->duplicate ) {
        Error("GeomDuplicate", "geometry has no duplicate method");
        return (GEOM *) nullptr;
    }

    GEOM *p = (GEOM *)malloc(sizeof(GEOM));
    GLOBAL_statistics_numberOfGeometries++;
    *p = *geom;
    p->obj = geom->methods->duplicate(geom->obj);

    return p;
}

/**
Will avoid intersection testing with geom1 and geom2 (possibly nullptr
pointers). Can be used for avoiding immediate selfintersections
*/
void
GeomDontIntersect(GEOM *geom1, GEOM *geom2) {
    excludedGeom1 = geom1;
    excludedGeom2 = geom2;
}

/**
This routine returns nullptr is the ray doesn't hit the discretisation of the
GEOMetry. If the ray hits the discretisation of the GEOM, containing
(among other information) the hit patch is returned.
The hitflags (defined in ray.h) determine whether the nearest intersection
is returned, or rather just any intersection (e.g. for shadow rays in
ray tracing or for form factor rays in radiosity), whether to consider
intersections with front/back facing patches and what other information
besides the hit patch (interpolated normal, intersection point, material
properties) to return.
*/
HITREC *
GeomDiscretisationIntersect(
    GEOM *geom,
    Ray *ray,
    float mindist,
    float *maxdist,
    int hitflags,
    HITREC *hitstore)
{
    Vector3D vtmp;
    float nmaxdist;

    if ( geom == excludedGeom1 || geom == excludedGeom2 ) {
        return (HITREC *) nullptr;
    }

    if ( geom->bounded ) {
        /* Check ray/bounding volume intersection */
        VECTORSUMSCALED(ray->pos, mindist, ray->dir, vtmp);
        if ( OutOfBounds(&vtmp, geom->bounds)) {
            nmaxdist = *maxdist;
            if ( !BoundsIntersect(ray, geom->bounds, mindist, &nmaxdist)) {
                return nullptr;
            }
        }
    }
    return geom->methods->discretisation_intersect(geom->obj, ray, mindist, maxdist, hitflags, hitstore);
}

HITLIST *
GeomAllDiscretisationIntersections(
    HITLIST *hits,
    GEOM *geom,
    Ray *ray,
    float mindist,
    float maxdist,
    int hitflags)
{
    Vector3D vtmp;
    float nmaxdist;

    if ( geom == excludedGeom1 || geom == excludedGeom2 ) {
        return hits;
    }

    if ( geom->bounded ) {
        // Check ray/bounding volume intersection
        VECTORSUMSCALED(ray->pos, mindist, ray->dir, vtmp);
        if ( OutOfBounds(&vtmp, geom->bounds)) {
            nmaxdist = maxdist;
            if ( !BoundsIntersect(ray, geom->bounds, mindist, &nmaxdist)) {
                return hits;
            }
        }
    }
    return geom->methods->all_discretisation_intersections(hits, geom->obj, ray, mindist, maxdist, hitflags);
}

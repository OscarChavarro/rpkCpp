#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "material/statistics.h"
#include "skin/Geometry.h"

Geometry *GLOBAL_geom_excludedGeom1 = (Geometry *) nullptr;
Geometry *GLOBAL_geom_excludedGeom2 = (Geometry *) nullptr;

static int globalCurrentMaxId = 0;

static void
boundsEnlargeTinyBit(float *bounds) {
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
This function is used to create a new geometry with given specific data and
methods. A pointer to the new geometry is returned

Note: currently containing the super() method.
*/
Geometry *
geomCreateBase(void *geometryData, GEOM_METHODS *methods) {
    if ( geometryData == nullptr) {
        return (Geometry *) nullptr;
    }

    Geometry *newGeometry = (Geometry *)malloc(sizeof(Geometry));
    GLOBAL_statistics_numberOfGeometries++;
    newGeometry->id = globalCurrentMaxId++;
    newGeometry->obj = geometryData;
    newGeometry->methods = methods;

    if ( methods->getBoundingBox ) {
        methods->getBoundingBox(geometryData, newGeometry->bounds);
        /* enlarge bounding box a tiny bit for more conservative bounding box culling */
        boundsEnlargeTinyBit(newGeometry->bounds);
        newGeometry->bounded = true;
    } else {
        BoundsInit(newGeometry->bounds);
        newGeometry->bounded = false;
    }
    newGeometry->shaftCullGeometry = false;

    newGeometry->radiance_data = (void *) nullptr;
    newGeometry->tmp.i = 0;
    newGeometry->omit = false;

    newGeometry->dlistid = -1;

    return newGeometry;
}

Geometry *
geomCreatePatchSetNew(java::ArrayList<PATCH *> *geometryList, GEOM_METHODS *methods) {
    PATCHLIST *patchList = nullptr;

    for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
        PATCHLIST *newNode = (PATCHLIST *)malloc(sizeof(PATCHLIST));
        newNode->next = patchList;
        newNode->patch = geometryList->get(i);
        patchList = newNode;
    }

    Geometry *newGeometry = geomCreatePatchSet(patchList, methods);
    if ( newGeometry != nullptr ) {
        newGeometry->newPatchSetData = geometryList;
    }
    return newGeometry;
}

Geometry *
geomCreatePatchSet(PATCHLIST *patchSet, GEOM_METHODS *methods) {
    if ( patchSet == nullptr ) {
        return nullptr;
    }

    Geometry *newGeometry = geomCreateBase(patchSet, methods);
    newGeometry->obj = patchSet;
    return newGeometry;
}

/**
This function prints the GEOMetry data to the file out
*/
void
geomPrint(FILE *out, Geometry *geom) {
    fprintf(out, "Geom %d, bounded = %s, shaftCullGeometry = %s:\n",
            geom->id,
            geom->bounded ? "TRUE" : "FALSE",
            geom->shaftCullGeometry ? "TRUE" : "FALSE");

    geom->methods->print(out, geom->obj);
}

/**
This function returns a bounding box for the GEOMetry
*/
float *
geomBounds(Geometry *geom) {
    return geom->bounds;
}

/**
This function destroys the given GEOMetry
*/
void
geomDestroy(Geometry *geom) {
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
geomIsAggregate(Geometry *geom) {
    return geom->methods->getPrimitiveGeometryChildrenList != (GEOMLIST *(*)(void *)) nullptr;
}

/**
Returns a linear list of the simpler GEOMEtries making up an aggregate GEOMetry.
A nullptr pointer is returned if the GEOMetry is a primitive
*/
GEOMLIST *
geomPrimList(Geometry *geom) {
    if ( geom->methods->getPrimitiveGeometryChildrenList ) {
        return geom->methods->getPrimitiveGeometryChildrenList(geom->obj);
    } else {
        return (GEOMLIST *) nullptr;
    }
}

/**
Returns a linear list of patches making up a primitive GEOMetry. A nullptr
pointer is returned if the given GEOMetry is an aggregate
*/
PATCHLIST *
geomPatchList(Geometry *geom) {
    if ( geom->methods->getPatchList ) {
        return geom->methods->getPatchList(geom->obj);
    } else {
        return (PATCHLIST *) nullptr;
    }
}

/**
This routine creates and returns a duplicate of the given geometry. Needed for
shaft culling.
*/
Geometry *
geomDuplicate(Geometry *geom) {
    if ( !geom->methods->duplicate ) {
        Error("geomDuplicate", "geometry has no duplicate method");
        return (Geometry *) nullptr;
    }

    Geometry *p = (Geometry *)malloc(sizeof(Geometry));
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
geomDontIntersect(Geometry *geom1, Geometry *geom2) {
    GLOBAL_geom_excludedGeom1 = geom1;
    GLOBAL_geom_excludedGeom2 = geom2;
}

/**
This routine returns nullptr is the ray doesn't hit the discretisation of the
GEOMetry. If the ray hits the discretisation of the Geometry, containing
(among other information) the hit patch is returned.
The hitflags (defined in ray.h) determine whether the nearest intersection
is returned, or rather just any intersection (e.g. for shadow rays in
ray tracing or for form factor rays in radiosity), whether to consider
intersections with front/back facing patches and what other information
besides the hit patch (interpolated normal, intersection point, material
properties) to return.
*/
HITREC *
geomDiscretizationIntersect(
        Geometry *geom,
        Ray *ray,
        float mindist,
        float *maxdist,
        int hitflags,
        HITREC *hitstore)
{
    Vector3D vtmp;
    float nmaxdist;

    if ( geom == GLOBAL_geom_excludedGeom1 || geom == GLOBAL_geom_excludedGeom2 ) {
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
    return geom->methods->discretizationIntersect(geom->obj, ray, mindist, maxdist, hitflags, hitstore);
}

HITLIST *
geomAllDiscretizationIntersections(
        HITLIST *hits,
        Geometry *geom,
        Ray *ray,
        float mindist,
        float maxdist,
        int hitflags)
{
    Vector3D vtmp;
    float nmaxdist;

    if ( geom == GLOBAL_geom_excludedGeom1 || geom == GLOBAL_geom_excludedGeom2 ) {
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
    return geom->methods->allDiscretizationIntersections(hits, geom->obj, ray, mindist, maxdist, hitflags);
}

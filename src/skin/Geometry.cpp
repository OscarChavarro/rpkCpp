#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "material/statistics.h"
#include "skin/Geometry.h"

Geometry *GLOBAL_geom_excludedGeom1 = nullptr;
Geometry *GLOBAL_geom_excludedGeom2 = nullptr;

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
static Geometry *
geomCreateBase(void *geometryData, GEOM_METHODS *methods) {
    if ( geometryData == nullptr) {
        return (Geometry *) nullptr;
    }

    Geometry *newGeometry = (Geometry *)malloc(sizeof(Geometry));
    GLOBAL_statistics_numberOfGeometries++;
    newGeometry->id = globalCurrentMaxId++;
    newGeometry->methods = methods;
    newGeometry->surfaceData = nullptr;
    newGeometry->compoundData = nullptr;
    newGeometry->aggregateData = nullptr;
    newGeometry->patchSetData = nullptr;

    if ( methods->getBoundingBox ) {
        methods->getBoundingBox(geometryData, newGeometry->bounds);
        /* enlarge bounding box a tiny bit for more conservative bounding box culling */
        boundsEnlargeTinyBit(newGeometry->bounds);
        newGeometry->bounded = true;
    } else {
        boundsInit(newGeometry->bounds);
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
geomCreatePatchSetNew(java::ArrayList<Patch *> *patchList, GEOM_METHODS *methods) {
    if ( patchList == nullptr ) {
        return nullptr;
    }
    Geometry *newGeometry = geomCreateBase(patchList, methods);
    newGeometry->patchSetData = patchList;
    return newGeometry;
}

Geometry *
geomCreateSurface(MeshSurface *surfaceData, GEOM_METHODS *methods) {
    if ( surfaceData == nullptr ) {
        return nullptr;
    }

    Geometry *newGeometry = geomCreateBase(surfaceData, methods);
    newGeometry->surfaceData = surfaceData;
    return newGeometry;
}

Geometry *
geomCreateCompound(Compound *compoundData, GEOM_METHODS *methods) {
    if ( compoundData == nullptr ) {
        return nullptr;
    }

    Geometry *newGeometry = geomCreateBase(compoundData, methods);
    newGeometry->compoundData = compoundData;
    newGeometry->aggregateData = &compoundData->children;
    return newGeometry;
}

Geometry *
geomCreateAggregateCompound(GeometryListNode *aggregateData, GEOM_METHODS *methods) {
    if ( aggregateData == nullptr ) {
        return nullptr;
    }

    Geometry *newGeometry = geomCreateBase(aggregateData, methods);
    newGeometry->aggregateData = aggregateData;
    return newGeometry;
}

/**
This function prints the geometry data to the file out
*/
void
geomPrint(FILE *out, Geometry *geom) {
    fprintf(out, "Geom %d, bounded = %s, shaftCullGeometry = %s:\n",
            geom->id,
            geom->bounded ? "TRUE" : "FALSE",
            geom->shaftCullGeometry ? "TRUE" : "FALSE");
}

/**
This function returns a bounding box for the geometry
*/
float *
geomBounds(Geometry *geom) {
    return geom->bounds;
}

/**
This function destroys the given geometry
*/
void
geomDestroy(Geometry *geom) {
    //geom->methods->destroy(geom->obj);
    free(geom);
    GLOBAL_statistics_numberOfGeometries--;
}

/**
This function returns nonzero if the given geometry is an aggregate. An
aggregate is a geometry that consists of simpler geometries. Currently,
there is only one type of aggregate geometry: the compound, which is basically
just a list of simpler geometries. Other aggregate geometries are also
possible, e.g. CSG objects. If the given geometry is a primitive, zero is
returned. A primitive geometry is a geometry that does not consist of
simpler geometries
*/
int
geomIsAggregate(Geometry *geom) {
    return geom->methods->getPrimitiveGeometryChildrenList != (GeometryListNode *(*)(void *)) nullptr;
}

/**
Returns a linear list of the simpler geometries making up an aggregate geometry.
A nullptr pointer is returned if the geometry is a primitive
*/
GeometryListNode *
geomPrimList(Geometry *geom) {
    if ( geomIsAggregate(geom) && geom->aggregateData != nullptr ) {
        return geom->aggregateData;
    } else {
        return (GeometryListNode *) nullptr;
    }
}

/**
Returns a linear list of patches making up a primitive geometry. A nullptr
pointer is returned if the given geometry is an aggregate
*/
java::ArrayList<Patch *> *
geomPatchList(Geometry *geom) {
    if ( geom->methods->getPatchList != nullptr ) {
        if ( geom->methods == &GLOBAL_skin_surfaceGeometryMethods ) {
            return geom->methods->getPatchList(geom->surfaceData);
        } else if ( geom->methods == &GLOBAL_skin_patchListGeometryMethods ) {
            return geom->patchSetData;
        } else if ( geom->methods == &GLOBAL_skin_compoundGeometryMethods ) {
            return geom->methods->getPatchList(geom->compoundData);
        }
    }
    return nullptr;
}

/**
This routine creates and returns a duplicate of the given geometry. Needed for
shaft culling.
*/
Geometry *
geomDuplicate(Geometry *geom) {
    if ( !geom->methods->duplicate ) {
        logError("geomDuplicate", "geometry has no duplicate method");
        return nullptr;
    }

    Geometry *p = (Geometry *)malloc(sizeof(Geometry));
    GLOBAL_statistics_numberOfGeometries++;
    *p = *geom;
    p->surfaceData = geom->surfaceData;
    p->compoundData = geom->compoundData;
    p->patchSetData = geom->patchSetData;
    p->aggregateData = geom->aggregateData;

    return p;
}

/**
Will avoid intersection testing with geom1 and geom2 (possibly nullptr
pointers). Can be used for avoiding immediate self-intersections
*/
void
geomDontIntersect(Geometry *geom1, Geometry *geom2) {
    GLOBAL_geom_excludedGeom1 = geom1;
    GLOBAL_geom_excludedGeom2 = geom2;
}

/**
This routine returns nullptr is the ray doesn't hit the discretization of the
geometry. If the ray hits the discretization of the Geometry, containing
(among other information) the hit patch is returned.
The hitFlags (defined in ray.h) determine whether the nearest intersection
is returned, or rather just any intersection (e.g. for shadow rays in
ray tracing or for form factor rays in radiosity), whether to consider
intersections with front/back facing patches and what other information
besides the hit patch (interpolated normal, intersection point, material
properties) to return.
*/
RayHit *
geomDiscretizationIntersect(
    Geometry *geom,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore)
{
    Vector3D vTmp;
    float nMaximumDistance;

    if ( geom == GLOBAL_geom_excludedGeom1 || geom == GLOBAL_geom_excludedGeom2 ) {
        return nullptr;
    }

    if ( geom->bounded ) {
        // Check ray/bounding volume intersection
        VECTORSUMSCALED(ray->pos, minimumDistance, ray->dir, vTmp);
        if ( OutOfBounds(&vTmp, geom->bounds)) {
            nMaximumDistance = *maximumDistance;
            if ( !boundsIntersect(ray, geom->bounds, minimumDistance, &nMaximumDistance)) {
                return nullptr;
            }
        }
    }

    if ( geom->surfaceData != nullptr ) {
        return geom->methods->discretizationIntersect(geom->surfaceData, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    } else if ( geom->compoundData != nullptr ) {
        return geom->methods->discretizationIntersect(geom->compoundData, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    } else if ( geom->patchSetData != nullptr ) {
        return geom->methods->discretizationIntersect(geom->patchSetData, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    } else if ( geom->aggregateData != nullptr ) {
        return geom->methods->discretizationIntersect(geom->aggregateData, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    }
    return nullptr;
}

HITLIST *
geomAllDiscretizationIntersections(
    HITLIST *hits,
    Geometry *geom,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags)
{
    Vector3D vTmp;
    float nMaximumDistance;

    if ( geom == GLOBAL_geom_excludedGeom1 || geom == GLOBAL_geom_excludedGeom2 ) {
        return hits;
    }

    if ( geom->bounded ) {
        // Check ray/bounding volume intersection
        VECTORSUMSCALED(ray->pos, minimumDistance, ray->dir, vTmp);
        if ( OutOfBounds(&vTmp, geom->bounds)) {
            nMaximumDistance = maximumDistance;
            if ( !boundsIntersect(ray, geom->bounds, minimumDistance, &nMaximumDistance)) {
                return hits;
            }
        }
    }

    if ( geom->surfaceData != nullptr ) {
        return geom->methods->allDiscretizationIntersections(hits, geom->surfaceData, ray, minimumDistance, maximumDistance, hitFlags);
    } else if ( geom->compoundData != nullptr ) {
        return geom->methods->allDiscretizationIntersections(hits, geom->compoundData, ray, minimumDistance, maximumDistance, hitFlags);
    } else if ( geom->patchSetData != nullptr ) {
        return geom->methods->allDiscretizationIntersections(hits, geom->patchSetData, ray, minimumDistance, maximumDistance, hitFlags);
    } else if ( geom->aggregateData != nullptr ) {
        return geom->methods->allDiscretizationIntersections(hits, geom->aggregateData, ray, minimumDistance, maximumDistance, hitFlags);
    }

    return nullptr;
}

int
Geometry::geomCountItems() {
    int count = 0;

    if ( geomIsAggregate(this)) {
        GeometryListNode *geometryList = geomPrimList(this);
        if ( geometryList != nullptr ) {
            GeometryListNode *window;
            for ( window = geometryList; window; window = window->next ) {
                Geometry *geometry = window->geom;
                count += geometry->geomCountItems();
            }
        }
    } else {
        count = 0;
	java::ArrayList<Patch *> *patches = geomPatchList(this);
        if ( patches != nullptr ) {
            count = patches->size();
        }
    }

    return this->tmp.i = count;
}

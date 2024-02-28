#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "material/statistics.h"
#include "skin/Geometry.h"

Geometry *GLOBAL_geom_excludedGeom1 = nullptr;
Geometry *GLOBAL_geom_excludedGeom2 = nullptr;

static int globalCurrentMaxId = 0;

Geometry::Geometry():
    id(),
    bounds(),
    radianceData(),
    displayListId(),
    itemCount(),
    bounded(),
    shaftCullGeometry(),
    omit(),
    className(),
    surfaceData(),
    compoundData(),
    patchSetData()
{
    className = GeometryClassId::UNDEFINED;
}

static bool contains(java::ArrayList<MeshSurface *> *deleted, MeshSurface *candidate) {
    for ( int i = 0; i < deleted->size(); i++ ) {
        if ( deleted->get(i) == candidate ) {
            return true;
        }
    }
    return false;
}

Geometry::~Geometry() {
    static java::ArrayList<MeshSurface *> deleted;

    if ( radianceData != nullptr && !isDuplicate ) {
        delete radianceData;
        radianceData = nullptr;
    }

    if ( surfaceData != nullptr && !isDuplicate ) {
        // TODO: Check why some elements are added twice to the main list
        if ( !contains(&deleted, surfaceData) ) {
            deleted.add(surfaceData);
            delete surfaceData;
        }
        surfaceData = nullptr;
    }

    if ( compoundData != nullptr && !isDuplicate ) {
        delete compoundData;
        compoundData = nullptr;
    }

    if ( patchSetData != nullptr && !isDuplicate ) {
        delete patchSetData;
        patchSetData = nullptr;
    }
}

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
geomCreateBase(
    PatchSet *patchSetData,
    MeshSurface *surfaceData,
    Compound *compoundData,
    GeometryClassId className)
{
    Geometry *newGeometry = new Geometry();
    GLOBAL_statistics_numberOfGeometries++;
    newGeometry->id = globalCurrentMaxId++;
    newGeometry->surfaceData = surfaceData;
    newGeometry->compoundData = compoundData;
    newGeometry->patchSetData = patchSetData;
    newGeometry->className = className;
    newGeometry->isDuplicate = false;

    if ( className == GeometryClassId::SURFACE_MESH ) {
        surfaceBounds(surfaceData, newGeometry->bounds);
    } else if ( className == GeometryClassId::COMPOUND ) {
        geometryListBounds(compoundData->children, newGeometry->bounds);
    } else /* if ( className == GeometryClassId::PATCH_SET ) */ {
        patchListBounds(patchSetData->patchList, newGeometry->bounds);
    }

    // Enlarge bounding box a tiny bit for more conservative bounding box culling
    boundsEnlargeTinyBit(newGeometry->bounds);
    newGeometry->bounded = true;
    newGeometry->shaftCullGeometry = false;
    newGeometry->radianceData = nullptr;
    newGeometry->itemCount = 0;
    newGeometry->omit = false;
    newGeometry->displayListId = -1;

    return newGeometry;
}

Geometry *
geomCreatePatchSet(java::ArrayList<Patch *> *geometryList) {
    PatchSet *patchSet = nullptr;

    if ( geometryList != nullptr && geometryList->size() > 0 ) {
        patchSet = new PatchSet(geometryList);
    }

    return geomCreatePatchSet(patchSet);
}

Geometry *
geomCreatePatchSet(PatchSet *patchSet) {
    if ( patchSet == nullptr ) {
        return nullptr;
    }

    Geometry *newGeometry = geomCreateBase(patchSet, nullptr, nullptr, GeometryClassId::PATCH_SET);
    return newGeometry;
}

Geometry *
geomCreateSurface(MeshSurface *surfaceData) {
    if ( surfaceData == nullptr ) {
        return nullptr;
    }

    Geometry *newGeometry = geomCreateBase(nullptr, surfaceData, nullptr, GeometryClassId::SURFACE_MESH);
    return newGeometry;
}

Geometry *
geomCreateCompound(Compound *compoundData) {
    if ( compoundData == nullptr ) {
        return nullptr;
    }

    Geometry *newGeometry = geomCreateBase(nullptr, nullptr, compoundData, GeometryClassId::COMPOUND);
    return newGeometry;
}

/**
This function returns a bounding box for the geometry
*/
float *
geomBounds(Geometry *geometry) {
    return geometry->bounds;
}

/**
This function destroys the given geometry
*/
void
geomDestroy(Geometry *geometry) {
    if ( geometry == nullptr ) {
        return;
    }
    delete geometry;
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
geomIsAggregate(Geometry *geometry) {
    return geometry != nullptr && (geometry->className == GeometryClassId::COMPOUND);
}

static java::ArrayList<Geometry *> *
cloneGeometryList(java::ArrayList<Geometry *> * input) {
    java::ArrayList<Geometry *> *output = new java::ArrayList<Geometry *>();
    for ( int i = 0; input != nullptr && i < input->size(); i++ ) {
        output->add(input->get(i));
    }
    return output;
}

/**
Returns a list of the simpler geometries making up an aggregate geometry.
A nullptr pointer is returned if the geometry is a primitive
*/
java::ArrayList<Geometry *> *
geomPrimListCopy(Geometry *geometry) {
    if ( geomIsAggregate(geometry) && geometry->compoundData != nullptr ) {
        return cloneGeometryList(geometry->compoundData->children);
    } else {
        return nullptr;
    }
}

java::ArrayList<Patch *> *
geomPatchArrayListReference(Geometry *geometry) {
    if ( geometry->className == GeometryClassId::SURFACE_MESH ) {
        return geometry->surfaceData->faces;
    } else if ( geometry->className == GeometryClassId::PATCH_SET ) {
        return geometry->patchSetData->patchList;
    } else if ( geometry->className == GeometryClassId::COMPOUND ) {
        return nullptr;
    }
    return nullptr;
}

/**
This routine creates and returns a duplicate of the given geometry. Needed for
shaft culling.
*/
Geometry *
geomDuplicate(Geometry *geometry) {
    if ( geometry->className != GeometryClassId::PATCH_SET ) {
        logError("geomDuplicate", "geometry has no duplicate method");
        return nullptr;
    }

    Geometry *newGeometry = new Geometry();
    GLOBAL_statistics_numberOfGeometries++;
    *newGeometry = *geometry;
    newGeometry->surfaceData = geometry->surfaceData;
    newGeometry->compoundData = geometry->compoundData;
    newGeometry->patchSetData = geometry->patchSetData;
    newGeometry->isDuplicate = true;

    return newGeometry;
}

/**
Will avoid intersection testing with geom1 and geom2 (possibly nullptr
pointers). Can be used for avoiding immediate self-intersections
*/
void
geomDontIntersect(Geometry *geometry1, Geometry *geometry2) {
    GLOBAL_geom_excludedGeom1 = geometry1;
    GLOBAL_geom_excludedGeom2 = geometry2;
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
    Geometry *geometry,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore)
{
    Vector3D vTmp;
    float nMaximumDistance;

    if ( geometry == GLOBAL_geom_excludedGeom1 || geometry == GLOBAL_geom_excludedGeom2 ) {
        return nullptr;
    }

    if ( geometry->bounded ) {
        // Check ray/bounding volume intersection
        VECTORSUMSCALED(ray->pos, minimumDistance, ray->dir, vTmp);
        if ( outOfBounds(&vTmp, geometry->bounds)) {
            nMaximumDistance = *maximumDistance;
            if ( !boundsIntersect(ray, geometry->bounds, minimumDistance, &nMaximumDistance)) {
                return nullptr;
            }
        }
    }

    if ( geometry->surfaceData != nullptr ) {
        return surfaceDiscretizationIntersect(geometry->surfaceData, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    } else if ( geometry->compoundData != nullptr ) {
        return geometry->compoundData->discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    } else if ( geometry->patchSetData != nullptr ) {
        RayHit *response;
        response = patchListIntersect(geometry->patchSetData->patchList, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
        return response;
    }
    return nullptr;
}

int
Geometry::geomCountItems() {
    int count = 0;

    if ( geomIsAggregate(this) ) {
        java::ArrayList<Geometry *> *geometryList = geomPrimListCopy(this);
        for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
            count += geometryList->get(i)->geomCountItems();
        }
        delete geometryList;
    } else {
        count = 0;

        java::ArrayList<Patch *> * list = geomPatchArrayListReference(this);
        if ( list != nullptr ) {
            count = (int)list->size();
        }
        //delete list;
    }

    return this->itemCount = count;
}

RayHit *
geometryListDiscretizationIntersect(
        java::ArrayList<Geometry *> *geometryList,
        Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore)
{
    RayHit *hit = nullptr;

    for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
        RayHit *h = geomDiscretizationIntersect(geometryList->get(i), ray, minimumDistance, maximumDistance, hitFlags, hitStore);
        if ( h != nullptr ) {
            if ( hitFlags & HIT_ANY ) {
                return h;
            } else {
                hit = h;
            }
        }
    }
    return hit;
}

/**
This function computes a bounding box for a list of geometries
*/
float *
geometryListBounds(java::ArrayList<Geometry *> *geometryList, float *boundingBox) {
    boundsInit(boundingBox);
    for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
        boundsEnlarge(boundingBox, geometryList->get(i)->bounds);
    }
    return boundingBox;
}

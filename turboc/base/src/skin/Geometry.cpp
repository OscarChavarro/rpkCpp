#include "java/util/ArrayList.txx"
#include "common/logging/Logger.h"
#include "common/statistics/Statistics.h"
#include "skin/Geometry.h"
#include "skin/MinMaxBox.h"
#include "skin/Compound.h"
#include "skin/MeshSurface.h"
#include "environment/geometry/elements/PatchSet.h"

Geometry *Geometry::excludedGeometry1 = NULL;
Geometry *Geometry::excludedGeometry2 = NULL;
int Geometry::nextGeometryId = 0;

Geometry::Geometry():
    id(),
    boundingBox(),
    rayIntersectionBox(),
    radianceData(),
    itemCount(),
    bounded(),
    omit(),
    isDuplicate(),
    className(UNDEFINED)
{
    shaftCullGeometry = false;
}

/**
This function is used to create a new geometry with given specific data and
methods
*/
Geometry::Geometry(
    GeometryClassId inClassName)
{
    Statistics::instance().reader.numberOfGeometries++;
    id = nextGeometryId;
    nextGeometryId++;
    className = inClassName;
    isDuplicate = false;
    bounded = false;
    shaftCullGeometry = false;
    rayIntersectionBox = NULL;
    radianceData = NULL;
    itemCount = 0;
    omit = false;
}

Geometry::~Geometry() {
    if ( rayIntersectionBox != NULL ) {
        delete rayIntersectionBox;
        rayIntersectionBox = NULL;
    }
    if ( radianceData != NULL && !isDuplicate ) {
        delete radianceData;
        radianceData = NULL;
    }
}

/**
This function returns a bounding box for the geometry
*/
BoundingBox
Geometry::getBoundingBox() const {
    return boundingBox;
}

MinMaxBox *
Geometry::getRayIntersectionBox() const {
    if ( rayIntersectionBox == NULL ) {
        rayIntersectionBox = new MinMaxBox(&boundingBox);
    } else {
        rayIntersectionBox->updateFromBoundingBox(&boundingBox);
    }
    return rayIntersectionBox;
}

/**
This function destroys the given geometry
*/
void
Geometry::destroy(Geometry *geometry) {
    if ( geometry == NULL ) {
        return;
    }
    delete geometry;
    Statistics::instance().reader.numberOfGeometries--;
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
bool
Geometry::isCompound() const {
    return className == COMPOUND;
}

ArrayList<Geometry *> *
Geometry::cloneGeometryList(const ArrayList<Geometry *> *input) {
    ArrayList<Geometry *> *output = new ArrayList<Geometry *>();
    for ( int i = 0; input != NULL && i < input->size(); i++ ) {
        output->add(input->get(i));
    }
    return output;
}

/**
Returns a list of the simpler geometries making up an aggregate geometry.
A NULL pointer is returned if the geometry is a primitive
*/
ArrayList<Geometry *> *
Geometry::primitiveListCopy(const Geometry *geometry) {
    if ( geometry->isCompound() ) {
        return Geometry::cloneGeometryList(((const Compound *)(geometry))->children);
    } else {
        return NULL;
    }
}

ArrayList<Patch *> *
Geometry::patchListReference(const Geometry *geometry) {
    if ( geometry->className == SURFACE_MESH ) {
        return ((const MeshSurface *)(geometry))->faces;
    } else if ( geometry->className == PATCH_SET ) {
        return ((const PatchSet *)(geometry))->getPatchList();
    } else if ( geometry->className == COMPOUND ) {
        return NULL;
    }
    return NULL;
}

/**
This routine creates and returns a duplicate of the given geometry. Needed for
shaft culling.
*/
Geometry *
Geometry::clone() const {
    if ( className != PATCH_SET ) {
        Logger::fatal(666, "duplicateIfPatchSet", "this should not happen");
    }

    PatchSet *newPatchSet = new PatchSet(Geometry::patchListReference(this));
    newPatchSet->id = Statistics::instance().reader.numberOfGeometries;
    newPatchSet->boundingBox = boundingBox;
    newPatchSet->radianceData = radianceData;
    newPatchSet->itemCount = itemCount;
    newPatchSet->bounded = bounded;
    newPatchSet->shaftCullGeometry = shaftCullGeometry;
    newPatchSet->omit = omit;
    newPatchSet->className = className;
    newPatchSet->isDuplicate = true;

    Statistics::instance().reader.numberOfGeometries++;

    return newPatchSet;
}

/**
Will avoid intersection testing with geom1 and geom2 (possibly NULL
pointers). Can be used for avoiding immediate self-intersections
*/
void
Geometry::dontIntersect(Geometry *geometry1, Geometry *geometry2) {
    Geometry::excludedGeometry1 = geometry1;
    Geometry::excludedGeometry2 = geometry2;
}

bool
Geometry::discretizationIntersectPreTest(
    const Ray *ray,
    float minimumDistance,
    const float *maximumDistance) const
{
    if ( this == excludedGeometry1 || this == excludedGeometry2 ) {
        return false;
    }

    if ( bounded ) {
        Vector3D vTmp;

        // Check ray/bounding volume intersection
        vTmp.sumScaled(ray->position, minimumDistance, ray->direction);
        if ( boundingBox.outOfBounds(&vTmp) ) {
            float nMaximumDistance = *maximumDistance;
            MinMaxBox *minMaxBox = getRayIntersectionBox();
            if ( !minMaxBox->intersect(ray, minimumDistance, &nMaximumDistance) ) {
                return false;
            }
        }
    }

    return true;
}

/**
This routine returns NULL is the ray doesn't hit the discretization of the
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
Geometry::discretizationIntersect(
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore) const
{
    if ( !discretizationIntersectPreTest(ray, minimumDistance, maximumDistance) ) {
        return NULL;
    }

    if ( className == SURFACE_MESH ) {
        return ((const MeshSurface *)(this))->discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    } else if ( className == COMPOUND ) {
        return ((const Compound *)(this))->discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    } else if ( className == PATCH_SET ) {
        return ((const PatchSet *)(this))->discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    }
    return NULL;
}

RayHit *
Geometry::listDiscretizationIntersect(
    const ArrayList<Geometry *> *geometryList,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore)
{
    RayHit *hit = NULL;

    for ( int i = 0; geometryList != NULL && i < geometryList->size(); i++ ) {
        RayHit *h = geometryList->get(i)->discretizationIntersect(
            ray, minimumDistance, maximumDistance, hitFlags, hitStore);
        if ( h != NULL ) {
            if ( hitFlags & ANY ) {
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
void
Geometry::listBounds(const ArrayList<Geometry *> *geometryList, BoundingBox *boundingBox) {
    for ( int i = 0; geometryList != NULL && i < geometryList->size(); i++ ) {
        boundingBox->enlarge(&geometryList->get(i)->boundingBox);
    }
}

/**
DiscretizationIntersect returns NULL is the ray doesn't hit the discretization
of the object. If the ray hits the object, a hit record is returned containing
information about the intersection point. See geometry.h for more explanation

Tests whether the Ray intersect the patches in the list. See geometry.h
(GeomDiscretizationIntersect()) for more explanation
*/
RayHit *
Geometry::patchListIntersect(
    const ArrayList<Patch *> *patchList,
    const Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore)
{
    RayHit *hit = NULL;
    for ( int i = 0; patchList != NULL && i < patchList->size(); i++ ) {
        RayHit *h = patchList->get(i)->intersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
        if ( h != NULL ) {
            if ( hitFlags & ANY ) {
                return h;
            } else {
                hit = h;
            }
        }
    }
    return hit;
}

BoundingBox *
Geometry::patchListBounds(const ArrayList<Patch *> *patchList, BoundingBox *boundingBox) {
    BoundingBox currentPatchBoundingBox;

    for ( int i = 0; patchList != NULL && i < patchList->size(); i++ ) {
        patchList->get(i)->computeAndGetBoundingBox(&currentPatchBoundingBox);
        boundingBox->enlarge(&currentPatchBoundingBox);
    }

    return boundingBox;
}

bool
Geometry::isExcluded() const {
    return this == excludedGeometry1 || this == excludedGeometry2;
}

#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/skin/Compound.h"
#include "vsdk/toolkit/skin/MeshSurface.h"
#include "vsdk/toolkit/environment/geometry/elements/PatchSet.h"

int RpkGeometry::nextGeometryId = 0;

RpkGeometry::RpkGeometry():
    id(),
    boundingBox(),
    radianceData(),
    itemCount(),
    bounded(),
    omit(),
    isDuplicate(),
    className()
{
    className = GeometryClassId::UNDEFINED;
    shaftCullGeometry = false;
}

/**
This function is used to create a new geometry with given specific data and
methods
*/
RpkGeometry::RpkGeometry(
    GeometryClassId inClassName)
{
    Statistics::instance().reader.numberOfGeometries++;
    id = nextGeometryId;
    nextGeometryId++;
    className = inClassName;
    isDuplicate = false;
    bounded = false;
    shaftCullGeometry = false;
    radianceData = nullptr;
    itemCount = 0;
    omit = false;
}

RpkGeometry::~RpkGeometry() {
    if ( radianceData != nullptr && !isDuplicate ) {
        delete radianceData;
        radianceData = nullptr;
    }
}

/**
This function returns a bounding box for the geometry
*/
RpkAxisAlignedBoundingBox
RpkGeometry::getBoundingBox() const {
    return boundingBox;
}

/**
This function destroys the given geometry
*/
void
RpkGeometry::destroy(RpkGeometry *geometry) {
    if ( geometry == nullptr ) {
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
RpkGeometry::isCompound() const {
    return className == GeometryClassId::COMPOUND;
}

java::ArrayList<RpkGeometry *> *
RpkGeometry::cloneGeometryList(const java::ArrayList<RpkGeometry *> *input) {
    java::ArrayList<RpkGeometry *> *output = new java::ArrayList<RpkGeometry *>();
    for ( int i = 0; input != nullptr && i < input->size(); i++ ) {
        output->add(input->get(i));
    }
    return output;
}

/**
Returns a list of the simpler geometries making up an aggregate geometry.
A nullptr pointer is returned if the geometry is a primitive
*/
java::ArrayList<RpkGeometry *> *
RpkGeometry::primitiveListCopy(const RpkGeometry *geometry) {
    if ( geometry->isCompound() ) {
        return RpkGeometry::cloneGeometryList(static_cast<const Compound *>(geometry)->children);
    } else {
        return nullptr;
    }
}

java::ArrayList<Patch *> *
RpkGeometry::patchListReference(const RpkGeometry *geometry) {
    if ( geometry->className == GeometryClassId::SURFACE_MESH ) {
        return static_cast<const MeshSurface *>(geometry)->faces;
    } else if ( geometry->className == GeometryClassId::PATCH_SET ) {
        return static_cast<const PatchSet *>(geometry)->getPatchList();
    } else if ( geometry->className == GeometryClassId::COMPOUND ) {
        return nullptr;
    }
    return nullptr;
}

/**
This routine creates and returns a duplicate of the given geometry. Needed for
shaft culling.
*/
RpkGeometry *
RpkGeometry::clone() const {
    if ( className != GeometryClassId::PATCH_SET ) {
        Logger::reportMessage("Geometry", Logger::FATAL_ERROR, "duplicateIfPatchSet", "this should not happen");
    }

    PatchSet *newPatchSet = new PatchSet(RpkGeometry::patchListReference(this));
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

bool
RpkGeometry::discretizationIntersectPreTest(
    const RpkRay *ray,
    float minimumDistance,
    const float *maximumDistance,
    const RayExclusionSet &exclusions) const
{
    if ( exclusions.excludesGeometry(this) ) {
        return false;
    }

    if ( bounded ) {
        Vector3D vTmp;

        // Check ray/bounding volume intersection
        vTmp.sumScaled(ray->position, minimumDistance, ray->direction);
        if ( boundingBox.outOfBounds(&vTmp) ) {
            float nMaximumDistance = *maximumDistance;
            if ( !boundingBox.intersect(ray, minimumDistance, &nMaximumDistance) ) {
                return false;
            }
        }
    }

    return true;
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
RpkRayHit *
RpkGeometry::discretizationIntersect(
    RpkRay *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RpkRayHit *hitStore,
    const RayExclusionSet &exclusions) const
{
    if ( !discretizationIntersectPreTest(ray, minimumDistance, maximumDistance, exclusions) ) {
        return nullptr;
    }

    if ( className == GeometryClassId::SURFACE_MESH ) {
        return static_cast<const MeshSurface *>(this)->discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
    } else if ( className == GeometryClassId::COMPOUND ) {
        return static_cast<const Compound *>(this)->discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
    } else if ( className == GeometryClassId::PATCH_SET ) {
        return static_cast<const PatchSet *>(this)->discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
    }
    return nullptr;
}

RpkRayHit *
RpkGeometry::listDiscretizationIntersect(
    const java::ArrayList<RpkGeometry *> *geometryList,
    RpkRay *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RpkRayHit *hitStore,
    const RayExclusionSet &exclusions)
{
    RpkRayHit *hit = nullptr;

    for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
        RpkRayHit *h = geometryList->get(i)->discretizationIntersect(
            ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
        if ( h != nullptr ) {
            if ( hitFlags & RayHitFlag::ANY ) {
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
RpkGeometry::listBounds(const java::ArrayList<RpkGeometry *> *geometryList, RpkAxisAlignedBoundingBox *boundingBox) {
    for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
        boundingBox->enlarge(&geometryList->get(i)->boundingBox);
    }
}

/**
DiscretizationIntersect returns nullptr is the ray doesn't hit the discretization
of the object. If the ray hits the object, a hit record is returned containing
information about the intersection point. See geometry.h for more explanation

Tests whether the Ray intersect the patches in the list. See geometry.h
(GeomDiscretizationIntersect()) for more explanation
*/
RpkRayHit *
RpkGeometry::patchListIntersect(
    const java::ArrayList<Patch *> *patchList,
    const RpkRay *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RpkRayHit *hitStore,
    const RayExclusionSet &exclusions)
{
    RpkRayHit *hit = nullptr;
    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        RpkRayHit *h = patchList->get(i)->intersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
        if ( h != nullptr ) {
            if ( hitFlags & RayHitFlag::ANY ) {
                return h;
            } else {
                hit = h;
            }
        }
    }
    return hit;
}

RpkAxisAlignedBoundingBox *
RpkGeometry::patchListBounds(const java::ArrayList<Patch *> *patchList, RpkAxisAlignedBoundingBox *boundingBox) {
    RpkAxisAlignedBoundingBox currentPatchBoundingBox;

    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        patchList->get(i)->computeBounds(&currentPatchBoundingBox);
        boundingBox->enlarge(&currentPatchBoundingBox);
    }

    return boundingBox;
}

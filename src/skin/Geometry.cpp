#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "material/statistics.h"
#include "skin/Geometry.h"

Geometry *GLOBAL_geom_excludedGeom1 = nullptr;
Geometry *GLOBAL_geom_excludedGeom2 = nullptr;

int Geometry::nextGeometryId = 0;

Geometry::Geometry():
    id(),
    boundingBox(),
    radianceData(),
    displayListId(),
    itemCount(),
    bounded(),
    omit(),
    isDuplicate(),
    className(),
    compoundData(),
    patchSetData()
{
    className = GeometryClassId::UNDEFINED;
    shaftCullGeometry = false;
}

/**
This function is used to create a new geometry with given specific data and
methods. A pointer to the new geometry is returned

Note: currently containing the super() method.
*/
Geometry::Geometry(
    PatchSet *patchSetData,
    Compound *compoundData,
    GeometryClassId className)
{
    GLOBAL_statistics.numberOfGeometries++;
    this->id = nextGeometryId++;
    this->compoundData = compoundData;
    this->patchSetData = patchSetData;
    this->className = className;
    this->isDuplicate = false;
    this->bounded = false;
    this->shaftCullGeometry = false;
    this->radianceData = nullptr;
    this->itemCount = 0;
    this->omit = false;
    this->displayListId = -1;

    if ( className == GeometryClassId::COMPOUND ) {
        geometryListBounds(compoundData->children, &this->boundingBox);
        this->boundingBox.enlargeTinyBit();
        this->bounded = true;
    } else if ( className == GeometryClassId::PATCH_SET && patchSetData != nullptr ) {
        patchListBounds(patchSetData->patchList, &this->boundingBox);
        this->boundingBox.enlargeTinyBit();
        this->bounded = true;
    }
}

bool
Geometry::contains(java::ArrayList<MeshSurface *> *deleted, MeshSurface *candidate) {
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

    if ( compoundData != nullptr && !isDuplicate ) {
        delete compoundData;
        compoundData = nullptr;
    }

    if ( patchSetData != nullptr && !isDuplicate ) {
        delete patchSetData;
        patchSetData = nullptr;
    }
}

Geometry *
geomCreatePatchSet(java::ArrayList<Patch *> *geometryList) {
    if ( geometryList != nullptr && geometryList->size() > 0 ) {
        return new Geometry(new PatchSet(geometryList), nullptr, GeometryClassId::PATCH_SET);
    }

    return nullptr;
}

Geometry *
geomCreateCompound(Compound *compoundData) {
    if ( compoundData == nullptr ) {
        return nullptr;
    }

    return new Geometry(nullptr, compoundData, GeometryClassId::COMPOUND);
}

/**
This function returns a bounding box for the geometry
*/
BoundingBox &
getBoundingBox(Geometry *geometry) {
    return geometry->boundingBox;
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
    GLOBAL_statistics.numberOfGeometries--;
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
    return className == GeometryClassId::COMPOUND;
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
    if ( geometry->isCompound() && geometry->compoundData != nullptr ) {
        return cloneGeometryList(geometry->compoundData->children);
    } else {
        return nullptr;
    }
}

java::ArrayList<Patch *> *
geomPatchArrayListReference(Geometry *geometry) {
    if ( geometry->className == GeometryClassId::SURFACE_MESH ) {
        return ((MeshSurface *)geometry)->faces;
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
geomDuplicateIfPatchSet(Geometry *geometry) {
    if ( geometry->className != GeometryClassId::PATCH_SET ) {
        logError("geomDuplicateIfPatchSet", "geometry has no duplicate method");
        return nullptr;
    }

    Geometry *newGeometry = new Geometry();
    GLOBAL_statistics.numberOfGeometries++;
    *newGeometry = *geometry;
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

bool
Geometry::discretizationIntersectPreTest(
    Ray *ray,
    float minimumDistance,
    const float *maximumDistance) const
{
    if ( this == GLOBAL_geom_excludedGeom1 || this == GLOBAL_geom_excludedGeom2 ) {
        return false;
    }

    if ( bounded ) {
        Vector3D vTmp;

        // Check ray/bounding volume intersection
        vectorSumScaled(ray->pos, minimumDistance, ray->dir, vTmp);
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
RayHit *
Geometry::discretizationIntersect(
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore) const
{
    if ( !discretizationIntersectPreTest(ray, minimumDistance, maximumDistance) ) {
        return nullptr;
    }

    if ( className == GeometryClassId::SURFACE_MESH ) {
        return ((MeshSurface *)this)->discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    } else if ( className == GeometryClassId::COMPOUND ) {
        return compoundData->discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    } else if ( className == GeometryClassId::PATCH_SET ) {
        return patchSetData->discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    }
    return nullptr;
}

int
Geometry::geomCountItems() {
    int count = 0;

    if ( isCompound() ) {
        if ( this->compoundData != nullptr && this->compoundData->children != nullptr ) {
            for ( int i = 0; i < this->compoundData->children->size(); i++ ) {
                count += this->compoundData->children->get(i)->geomCountItems();
            }
        }
    } else {
        java::ArrayList<Patch *> * list = geomPatchArrayListReference(this);
        if ( list != nullptr ) {
            count = (int)list->size();
        }
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
        RayHit *h = geometryList->get(i)->discretizationIntersect(
            ray, minimumDistance, maximumDistance, hitFlags, hitStore);
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
void
geometryListBounds(java::ArrayList<Geometry *> *geometryList, BoundingBox *boundingBox) {
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
RayHit *
Geometry::patchListIntersect(
    java::ArrayList<Patch *> *patchList,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore)
{
    RayHit *hit = nullptr;
    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        RayHit *h = patchList->get(i)->intersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
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

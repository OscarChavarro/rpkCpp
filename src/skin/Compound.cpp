#include "material/statistics.h"
#include "skin/Compound.h"

/**
Creates a Compound from a list of geometries

This function creates a Compound from a linear list of geometries.
Actually, it just counts the number of compounds in the scene and
returns the geometryList. You can as well directly globalPass 'geometryList'
to geomCreateBase() for creating a Compound Geometry if you don't want
it to be counted
*/
Compound *
compoundCreate(GeometryListNode *geometryList) {
    GLOBAL_statistics_numberOfCompounds++;
    Compound *group = new Compound();
    group->children = *geometryList;
    return group;
}

/**
This method will compute a bounding box for a geometry. The bounding box
is filled in bounding box and a pointer to the filled in bounding box returned
*/
float *
compoundBounds(Compound *obj, float *boundingBox) {
    return geometryListBounds(&obj->children, boundingBox);
}

/**
Returns the list of children geometries if the geometry is an aggregate
*/
static GeometryListNode *
compoundPrimitives(Compound *obj) {
    return &obj->children;
}

/**
DiscretizationIntersect returns nullptr is the ray doesn't hit the discretization
of the object. If the ray hits the object, a hit record is returned containing
information about the intersection point. See geometry.h for more explanation
*/
RayHit *
compoundDiscretizationIntersect(
    Compound *obj,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore)
{
    return geometryListDiscretizationIntersect(
        &obj->children, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
}

RayHit *
aggregationDiscretizationIntersect(
    GeometryListNode *obj,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore)
{
    return geometryListDiscretizationIntersect(
            obj, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
}

static HITLIST *
compoundAllDiscretizationIntersections(
    HITLIST *hits,
    Compound *obj,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags)
{
    return geomListAllDiscretizationIntersections(hits, &obj->children, ray, minimumDistance, maximumDistance, hitFlags);
}

// A set of pointers to the functions (methods) to operate on compounds
GEOM_METHODS GLOBAL_skin_compoundGeometryMethods = {
    (GeometryListNode *(*)(void *)) compoundPrimitives,
    (HITLIST *(*)(HITLIST *, void *, Ray *, float, float, int)) compoundAllDiscretizationIntersections
};

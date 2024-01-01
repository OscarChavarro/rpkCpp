#include <cstdio>

#include "material/statistics.h"
#include "skin/Geometry.h"
#include "skin/Compound.h"

/**
Creates a Compound from a list of geometries

This function creates a Compound from a linear list of geometries.
Actually, it just counts the number of compounds in the scene and
returns the geometryList. You can as well directly globalPass 'geometryList'
to geomCreateBase() for creating a Compound Geometry if you don't want
it to be counted
*/
COMPOUND *
compoundCreate(GEOMLIST *geomlist) {
    GLOBAL_statistics_numberOfCompounds++;
    return geomlist;
}

/**
This method will compute a bounding box for a geometry. The bounding box
is filled in bounding box and a pointer to the filled in bounding box returned
*/
static float *
compoundBounds(COMPOUND *obj, float *boundingbox) {
    return GeomListBounds(obj, boundingbox);
}

/**
This method will destroy the geometry and it's children geometries if any
*/
static void
compoundDestroy(COMPOUND *obj) {
    GeomListIterate(obj, geomDestroy);
    GeomListDestroy(obj);
    GLOBAL_statistics_numberOfCompounds--;
}

/**
This method will print the geometry to the file out
*/
static void
compoundPrint(FILE *out, COMPOUND *obj) {
    fprintf(out, "compound\n");
    GeomListIterate1B(obj, geomPrint, out);
    fprintf(out, "end of compound\n");
}

/**
Returns the list of children geometries if the geometry is an aggregate
*/
static GEOMLIST *
compoundPrimitives(COMPOUND *obj) {
    return obj;
}

static HITREC *
compoundDiscretizationIntersect(
    COMPOUND *obj,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    HITREC *hitStore)
{
    return GeomListDiscretisationIntersect(obj, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
}

static HITLIST *
compoundAllDiscretizationIntersections(
    HITLIST *hits,
    COMPOUND *obj,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags)
{
    return GeomListAllDiscretisationIntersections(hits, obj, ray, minimumDistance, maximumDistance, hitFlags);
}

// A set of pointers to the functions (methods) to operate on compounds
GEOM_METHODS GLOBAL_skin_compoundGeometryMethods = {
        (float *(*)(void *, float *)) compoundBounds,
        (void (*)(void *)) compoundDestroy,
        (void (*)(FILE *, void *)) compoundPrint,
        (GEOMLIST *(*)(void *)) compoundPrimitives,
    (PATCHLIST *(*)(void *)) nullptr,
        (HITREC *(*)(void *, Ray *, float, float *, int, HITREC *)) compoundDiscretizationIntersect,
        (HITLIST *(*)(HITLIST *, void *, Ray *, float, float, int)) compoundAllDiscretizationIntersections,
    (void *(*)(void *)) nullptr
};

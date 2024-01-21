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
static float *
compoundBounds(Compound *obj, float *boundingBox) {
    return geometryListBounds(&obj->children, boundingBox);
}

/**
This method will destroy the geometry and it's children geometries if any
*/
static void
compoundDestroy(Compound *obj) {
    GeometryListNode *window = &obj->children;
    Geometry *geometry;
    while (window) {
        geometry = window->geom;
        window = window->next;
        geomDestroy(geometry);
    }

    window = &obj->children;
    while ( window ) {
        GeometryListNode *nextNode = window->next;
        free(window);
        window = nextNode;
    }

    GLOBAL_statistics_numberOfCompounds--;
}

/**
This method will printRegularHierarchy the geometry to the file out
*/
static void
compoundPrint(FILE *out, Compound *compound) {
    fprintf(out, "compound\n");
    GeometryListNode *window = &compound->children;
    Geometry *geometry;
    while (window) {
        geometry = window->geom;
        window = window->next;
        geomPrint(out, geometry);
    }
    fprintf(out, "end of compound\n");
}

/**
Returns the list of children geometries if the geometry is an aggregate
*/
static GeometryListNode *
compoundPrimitives(Compound *obj) {
    return &obj->children;
}

static RayHit *
compoundDiscretizationIntersect(
        Compound *obj,
        Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore)
{
    return geometryListDiscretizationIntersect(&obj->children, ray, minimumDistance, maximumDistance, hitFlags,
                                               hitStore);
}

static HITLIST *
compoundAllDiscretizationIntersections(
        HITLIST *hits,
        Compound *obj,
        Ray *ray,
        float minimumDistance,
        float maximumDistance,
        int hitFlags) {
    return geomListAllDiscretizationIntersections(hits, &obj->children, ray, minimumDistance, maximumDistance, hitFlags);
}

// A set of pointers to the functions (methods) to operate on compounds
GEOM_METHODS GLOBAL_skin_compoundGeometryMethods = {
        (float *(*)(void *, float *)) compoundBounds,
        (void (*)(void *)) compoundDestroy,
        (void (*)(FILE *, void *)) compoundPrint,
        (GeometryListNode *(*)(void *)) compoundPrimitives,
        (PatchSet *(*)(void *)) nullptr,
        (RayHit *(*)(void *, Ray *, float, float *, int, RayHit *)) compoundDiscretizationIntersect,
        (HITLIST *(*)(HITLIST *, void *, Ray *, float, float, int)) compoundAllDiscretizationIntersections,
        (void *(*)(void *)) nullptr
};

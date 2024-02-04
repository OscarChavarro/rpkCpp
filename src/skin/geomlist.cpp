#include "java/util/ArrayList.txx"
#include "skin/Geometry.h"
#include "skin/geomlist.h"

GeometryListNode *
geometryListAdd(GeometryListNode *geometryList, Geometry *geometry) {
    if ( geometry == nullptr) {
        return geometryList;
    }

    GeometryListNode *newList = (GeometryListNode *)malloc(sizeof(LIST));
    newList->geometry = geometry;
    newList->next = geometryList;

    return newList;
}

/**
This function computes a bounding box for a list of geometries
*/
float *
geometryListBounds(GeometryListNode *geometryList, float *boundingBox) {
    boundsInit(boundingBox);
    for ( GeometryListNode *window = geometryList; window != nullptr; window = window->next ) {
        boundsEnlarge(boundingBox, window->geometry->bounds);
    }
    return boundingBox;
}

/**
Builds a linear list of patches making up all the geometries in the list, whether
they are primitive or not
*/
void
buildPatchList(GeometryListNode *geometryList, java::ArrayList<Patch *> *patchList) {
    if ( geometryList == nullptr ) {
        return;
    }

    for ( GeometryListNode *window = geometryList; window != nullptr; window = window->next ) {
        Geometry *geometry = window->geometry;
        if ( geomIsAggregate(geometry) ) {
            buildPatchList(geomPrimList(geometry), patchList);
        } else {
            java::ArrayList<Patch *> *list2 = geomPatchArrayList(geometry);

            for ( int i = 0; list2 != nullptr && i < list2->size(); i++ ) {
                Patch *patch = list2->get(i);
                if ( patch != nullptr ) {
                    patchList->add(0, patch);
                }
            }
        }
    }
}

RayHit *
geometryListDiscretizationIntersect(
    GeometryListNode *geometryList,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore)
{
    RayHit *hit = nullptr;

    for ( GeometryListNode *window = geometryList; window; window = window->next ) {
        Geometry *geometry = window->geometry;
        RayHit *h = geomDiscretizationIntersect(geometry, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
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

HITLIST *
geomListAllDiscretizationIntersections(
    HITLIST *hits,
    GeometryListNode *geometryList,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags)
{
    for ( GeometryListNode *window = geometryList; window != nullptr; window = window->next ) {
        hits = geomAllDiscretizationIntersections(hits, window->geometry, ray, minimumDistance, maximumDistance, hitFlags);
    }
    return hits;
}

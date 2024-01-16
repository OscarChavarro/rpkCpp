#include "skin/PatchSet.h"
#include "skin/geomlist.h"
#include "skin/Geometry.h"
#include "skin/hitlist.h"

GeometryListNode *
geometryListAdd(GeometryListNode *geometryList, Geometry *geometry) {
    if ( geometry == nullptr) {
        return geometryList;
    }

    GeometryListNode *newList = (GeometryListNode *)malloc(sizeof(LIST));
    newList->geom = geometry;
    newList->next = geometryList;

    return newList;
}

/**
This function computes a bounding box for a list of geometries
*/
float *
geometryListBounds(GeometryListNode *geometryList, float *boundingBox) {
    boundsInit(boundingBox);
    if ( geometryList != nullptr ) {
        for ( GeometryListNode *window = geometryList; window != nullptr; window = window->next ) {
            Geometry *geometry = window->geom;
            boundsEnlarge(boundingBox, geomBounds(geometry));
        }
    }
    return boundingBox;
}

/**
Builds a linear list of PATCHES making up all the geometries in the list, whether
they are primitive or not
*/
PatchSet *
buildPatchList(GeometryListNode *geometryList, PatchSet *patchList) {
    if ( geometryList != nullptr ) {
        GeometryListNode *geometryWindow;
        for ( geometryWindow = geometryList; geometryWindow; geometryWindow = geometryWindow->next ) {
            Geometry *geometry = geometryWindow->geom;
            if ( geomIsAggregate(geometry)) {
                patchList = buildPatchList(geomPrimList(geometry), patchList);
            } else {
                PatchSet *list1 = patchList;
                PatchSet *list2 = geomPatchList(geometry);

                for ( PatchSet *patchWindow = list2; patchWindow != nullptr; patchWindow = patchWindow->next ) {
                    if ( patchWindow->patch != nullptr ) {
                        PatchSet *newListNode = (PatchSet *)malloc(sizeof(PatchSet));
                        newListNode->patch = patchWindow->patch;
                        newListNode->next = list1;
                        list1 = newListNode;
                    }
                }
                patchList = list1;
            }
        }
    }

    return patchList;
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
    RayHit *h;
    RayHit *hit;

    hit = (RayHit *) nullptr;
    if ( geometryList != nullptr ) {
        GeometryListNode *window;
        for ( window = geometryList; window; window = window->next ) {
            Geometry *geometry = window->geom;
            if ((h = geomDiscretizationIntersect(geometry, ray, minimumDistance, maximumDistance, hitFlags, hitStore))) {
                if ( hitFlags & HIT_ANY ) {
                    return h;
                } else {
                    hit = h;
                }
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
    if ( geometryList != nullptr ) {
        GeometryListNode *window;
        for ( window = geometryList; window; window = window->next ) {
            Geometry *geometry = window->geom;
            hits = geomAllDiscretizationIntersections(hits, geometry, ray, minimumDistance, maximumDistance, hitFlags);
        }
    }
    return hits;
}

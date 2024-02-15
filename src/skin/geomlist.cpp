#include "java/util/ArrayList.txx"
#include "skin/geomlist.h"

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

#include "java/util/ArrayList.txx"
#include "skin/PatchSet.h"

PatchSet::PatchSet(const ArrayList<Patch *> *input): Geometry(PATCH_SET) {
    patchList = new ArrayList<Patch *>();
    for ( int i = 0; input != NULL && i < input->size(); i++ ) {
        patchList->add(input->get(i));
    }

    Geometry::patchListBounds(getPatchList(), &boundingBox);
    boundingBox.enlargeTinyBit();
    bounded = true;
}

PatchSet::~PatchSet() {
    if ( !isDuplicate && patchList != NULL ) {
        delete patchList;
        patchList = NULL;
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
PatchSet::discretizationIntersect(
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore) const
{
    if ( !discretizationIntersectPreTest(ray, minimumDistance, maximumDistance) ) {
        return NULL;
    }

    return Geometry::patchListIntersect(patchList, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
}

ArrayList<Patch *> *
PatchSet::getPatchList() const {
    return patchList;
}

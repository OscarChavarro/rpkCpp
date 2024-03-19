#include "java/util/ArrayList.txx"
#include "skin/PatchSet.h"

PatchSet::PatchSet(java::ArrayList<Patch *> *input) {
    patchList = new java::ArrayList<Patch *>();
    for ( int i = 0; input != nullptr && i < input->size(); i++ ) {
        patchList->add(input->get(i));
    }
}

PatchSet::~PatchSet() {
    for ( int i = 0; i < patchList->size(); i++ ) {
        //delete patchList->get(i);
    }
    delete patchList;
    patchList = nullptr;
}

/**
DiscretizationIntersect returns nullptr is the ray doesn't hit the discretization
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
    return patchListIntersect(patchList, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
}

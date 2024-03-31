#include "java/util/ArrayList.txx"
#include "skin/PatchSet.h"

PatchSet::PatchSet(java::ArrayList<Patch *> *input): Geometry(nullptr, GeometryClassId::PATCH_SET) {
    patchList = new java::ArrayList<Patch *>();
    for ( int i = 0; input != nullptr && i < input->size(); i++ ) {
        patchList->add(input->get(i));
    }
    //patchSetData = this;
    //patchListBounds(patchList, &this->boundingBox);
    //boundingBox.enlargeTinyBit();
    //bounded = true;
}

PatchSet::~PatchSet() {
    for ( int i = 0; i < patchList->size(); i++ ) {
        //delete patchList->get(i);
    }
    delete patchList;
    patchList = nullptr;
}

/**
Computes a bounding box for the given list of patches. The bounding box is
filled in 'bounding box' and a pointer to it returned
*/
BoundingBox *
patchListBounds(java::ArrayList<Patch *> *patchList, BoundingBox *boundingBox) {
    BoundingBox currentPatchBoundingBox;

    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        patchList->get(i)->getBoundingBox(&currentPatchBoundingBox);
        boundingBox->enlarge(&currentPatchBoundingBox);
    }

    return boundingBox;
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
    return Geometry::patchListIntersect(patchList, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
}

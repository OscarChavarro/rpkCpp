#include "java/util/ArrayList.txx"
#include "skin/PatchSet.h"

PatchSet::PatchSet(const java::ArrayList<Patch *> *input): Geometry(nullptr, nullptr, GeometryClassId::PATCH_SET) {
    patchList = new java::ArrayList<Patch *>();
    for ( int i = 0; input != nullptr && i < input->size(); i++ ) {
        patchList->add(input->get(i));
    }
}

PatchSet::~PatchSet() {
    if ( !isDuplicate && patchList != nullptr ) {
        delete patchList;
        patchList = nullptr;
    }
}

/**
Computes a bounding box for the given list of patches. The bounding box is
filled in 'bounding box' and a pointer to it returned
*/
BoundingBox *
patchListBounds(const java::ArrayList<Patch *> *patchList, BoundingBox *boundingBox) {
    BoundingBox currentPatchBoundingBox;

    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        patchList->get(i)->computeAndGetBoundingBox(&currentPatchBoundingBox);
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

java::ArrayList<Patch *> *
PatchSet::getPatchList() const {
    return patchSetData->patchList;
}

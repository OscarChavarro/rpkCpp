#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/elements/PatchSet.h"

PatchSet::PatchSet(const java::ArrayList<Patch *> *input): RpkGeometry(GeometryClassId::PATCH_SET) {
    memoryPoolManaged = false;
    patchList = new java::ArrayList<Patch *>();
    for ( int i = 0; input != nullptr && i < input->size(); i++ ) {
        patchList->add(input->get(i));
    }

    RpkGeometry::patchListBounds(getPatchList(), &boundingBox);
    boundingBox.enlargeTinyBit();
    bounded = true;
}

PatchSet::PatchSet(const java::ArrayList<Patch *> *input, const RpkAxisAlignedBoundingBox &patchBounds):
    RpkGeometry(GeometryClassId::PATCH_SET)
{
    memoryPoolManaged = false;
    patchList = new java::ArrayList<Patch *>();
    for ( int i = 0; input != nullptr && i < input->size(); i++ ) {
        patchList->add(input->get(i));
    }

    boundingBox.enlarge(&patchBounds);
    boundingBox.enlargeTinyBit();
    bounded = true;
}

PatchSet::~PatchSet() {
    // PatchSet always owns its internal patch pointer container.
    // `isDuplicate` only controls shared resources at Geometry level
    // (for example radianceData), not this list's lifetime.
    if ( patchList != nullptr ) {
        delete patchList;
        patchList = nullptr;
    }
}

/**
DiscretizationIntersect returns nullptr is the ray doesn't hit the discretization
of the object. If the ray hits the object, a hit record is returned containing
information about the intersection point. See geometry.h for more explanation

Tests whether the Ray intersect the patches in the list. See geometry.h
(GeomDiscretizationIntersect()) for more explanation
*/
RpkRayHit *
PatchSet::discretizationIntersect(
    RpkRay *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RpkRayHit *hitStore,
    const RayExclusionSet &exclusions) const
{
    if ( !discretizationIntersectPreTest(ray, minimumDistance, maximumDistance, exclusions) ) {
        return nullptr;
    }

    return RpkGeometry::patchListIntersect(patchList, ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
}

java::ArrayList<Patch *> *
PatchSet::getPatchList() const {
    return patchList;
}

bool
PatchSet::isMemoryPoolManaged() const {
    return memoryPoolManaged;
}

void
PatchSet::setMemoryPoolManaged(const bool value) {
    memoryPoolManaged = value;
}

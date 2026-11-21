#ifndef RAY_EXCLUSION_SET__
#define RAY_EXCLUSION_SET__

#include "vsdk/toolkit/environment/geometry/elements/PatchConstants.h"

class Patch;
class RpkGeometry;

/**
Patches and geometries that a visibility query must ignore, typically the
source and receiver of a form factor (and their twins), to avoid immediate
self-intersections. The value is built by the caller and passed down every
intersect call, so two queries never see each other's exclusions.

An excluded geometry (a source or receiver cluster) excludes every face
below it, wherever the spatial index stores that face: as part of the
geometry, of a sub-geometry, or as a loose patch. Since faces are numbered
depth-first over the cluster hierarchy (Scene::buildFaceTable()), the faces
below a geometry are a contiguous interval of face indices, so the face test
is two comparisons per excluded geometry and needs no pointers.

Shared, read-only: it has no setters, and every query is const.
*/
class RayExclusionSet {
  private:
    const Patch *excludedPatch[MAX_EXCLUDED_PATCHES];
    const RpkGeometry *excludedGeometry1;
    const RpkGeometry *excludedGeometry2;
    // Face index intervals of the excluded geometries, empty when first > last
    int excludedFirstFace1;
    int excludedLastFace1;
    int excludedFirstFace2;
    int excludedLastFace2;

  public:
    inline RayExclusionSet();
    inline RayExclusionSet(const Patch *p0, const Patch *p1);
    inline RayExclusionSet(const Patch *p0, const Patch *p1, const Patch *p2);
    RayExclusionSet(
        const Patch *p0,
        const Patch *p1,
        const Patch *p2,
        const Patch *p3,
        const RpkGeometry *geometry1,
        const RpkGeometry *geometry2);

    inline bool excludesPatch(const Patch *patch) const;
    inline bool excludesGeometry(const RpkGeometry *geometry) const;
    inline bool excludesFace(int faceIndex) const;
    inline const Patch *getExcludedPatch(int index) const;
    inline const RpkGeometry *getExcludedGeometry1() const;
    inline const RpkGeometry *getExcludedGeometry2() const;
};

inline
RayExclusionSet::RayExclusionSet():
    excludedPatch(),
    excludedGeometry1(nullptr),
    excludedGeometry2(nullptr),
    excludedFirstFace1(0),
    excludedLastFace1(-1),
    excludedFirstFace2(0),
    excludedLastFace2(-1)
{
    for ( int i = 0; i < MAX_EXCLUDED_PATCHES; i++ ) {
        excludedPatch[i] = nullptr;
    }
}

inline
RayExclusionSet::RayExclusionSet(const Patch *p0, const Patch *p1):
    excludedPatch(),
    excludedGeometry1(nullptr),
    excludedGeometry2(nullptr),
    excludedFirstFace1(0),
    excludedLastFace1(-1),
    excludedFirstFace2(0),
    excludedLastFace2(-1)
{
    excludedPatch[0] = p0;
    excludedPatch[1] = p1;
    excludedPatch[2] = nullptr;
    excludedPatch[3] = nullptr;
}

inline
RayExclusionSet::RayExclusionSet(const Patch *p0, const Patch *p1, const Patch *p2):
    excludedPatch(),
    excludedGeometry1(nullptr),
    excludedGeometry2(nullptr),
    excludedFirstFace1(0),
    excludedLastFace1(-1),
    excludedFirstFace2(0),
    excludedLastFace2(-1)
{
    excludedPatch[0] = p0;
    excludedPatch[1] = p1;
    excludedPatch[2] = p2;
    excludedPatch[3] = nullptr;
}

inline bool
RayExclusionSet::excludesPatch(const Patch *patch) const {
    for ( int i = 0; i < MAX_EXCLUDED_PATCHES; i++ ) {
        if ( excludedPatch[i] == patch ) {
            return true;
        }
    }
    return false;
}

inline bool
RayExclusionSet::excludesGeometry(const RpkGeometry *geometry) const {
    return geometry == excludedGeometry1 || geometry == excludedGeometry2;
}

/**
True when the face lies below one of the excluded geometries
*/
inline bool
RayExclusionSet::excludesFace(const int faceIndex) const {
    return (faceIndex >= excludedFirstFace1 && faceIndex <= excludedLastFace1)
        || (faceIndex >= excludedFirstFace2 && faceIndex <= excludedLastFace2);
}

inline const Patch *
RayExclusionSet::getExcludedPatch(const int index) const {
    return excludedPatch[index];
}

inline const RpkGeometry *
RayExclusionSet::getExcludedGeometry1() const {
    return excludedGeometry1;
}

inline const RpkGeometry *
RayExclusionSet::getExcludedGeometry2() const {
    return excludedGeometry2;
}

#endif

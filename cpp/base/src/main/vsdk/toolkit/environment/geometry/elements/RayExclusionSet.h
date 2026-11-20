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

Shared, read-only: it has no setters, and every query is const.
*/
class RayExclusionSet {
  private:
    const Patch *excludedPatch[MAX_EXCLUDED_PATCHES];
    const RpkGeometry *excludedGeometry1;
    const RpkGeometry *excludedGeometry2;

  public:
    inline RayExclusionSet();
    inline RayExclusionSet(const Patch *p0, const Patch *p1);
    inline RayExclusionSet(const Patch *p0, const Patch *p1, const Patch *p2);
    inline RayExclusionSet(
        const Patch *p0,
        const Patch *p1,
        const Patch *p2,
        const Patch *p3,
        const RpkGeometry *geometry1,
        const RpkGeometry *geometry2);

    inline bool excludesPatch(const Patch *patch) const;
    inline bool excludesGeometry(const RpkGeometry *geometry) const;
    inline const Patch *getExcludedPatch(int index) const;
    inline const RpkGeometry *getExcludedGeometry1() const;
    inline const RpkGeometry *getExcludedGeometry2() const;
};

inline
RayExclusionSet::RayExclusionSet():
    excludedPatch(),
    excludedGeometry1(nullptr),
    excludedGeometry2(nullptr)
{
    for ( int i = 0; i < MAX_EXCLUDED_PATCHES; i++ ) {
        excludedPatch[i] = nullptr;
    }
}

inline
RayExclusionSet::RayExclusionSet(const Patch *p0, const Patch *p1):
    excludedPatch(),
    excludedGeometry1(nullptr),
    excludedGeometry2(nullptr)
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
    excludedGeometry2(nullptr)
{
    excludedPatch[0] = p0;
    excludedPatch[1] = p1;
    excludedPatch[2] = p2;
    excludedPatch[3] = nullptr;
}

inline
RayExclusionSet::RayExclusionSet(
    const Patch *p0,
    const Patch *p1,
    const Patch *p2,
    const Patch *p3,
    const RpkGeometry *geometry1,
    const RpkGeometry *geometry2):
    excludedPatch(),
    excludedGeometry1(geometry1),
    excludedGeometry2(geometry2)
{
    excludedPatch[0] = p0;
    excludedPatch[1] = p1;
    excludedPatch[2] = p2;
    excludedPatch[3] = p3;
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

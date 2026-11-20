#ifndef __RAY_EXCLUSION_SET__
#define __RAY_EXCLUSION_SET__

#include "vsdk/toolkit/environment/geometry/elements/PatchConstants.h"

class Patch;
class Geometry;

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
    const Geometry *excludedGeometry1;
    const Geometry *excludedGeometry2;

  public:
    inline RayExclusionSet();
    inline RayExclusionSet(const Patch *p0, const Patch *p1);
    inline RayExclusionSet(const Patch *p0, const Patch *p1, const Patch *p2);
    inline RayExclusionSet(
        const Patch *p0,
        const Patch *p1,
        const Patch *p2,
        const Patch *p3,
        const Geometry *geometry1,
        const Geometry *geometry2);

    inline bool excludesPatch(const Patch *patch) const;
    inline bool excludesGeometry(const Geometry *geometry) const;
};

inline
RayExclusionSet::RayExclusionSet():
    excludedPatch(),
    excludedGeometry1(NULL),
    excludedGeometry2(NULL)
{
    for ( int i = 0; i < MAX_EXCLUDED_PATCHES; i++ ) {
        excludedPatch[i] = NULL;
    }
}

inline
RayExclusionSet::RayExclusionSet(const Patch *p0, const Patch *p1):
    excludedPatch(),
    excludedGeometry1(NULL),
    excludedGeometry2(NULL)
{
    excludedPatch[0] = p0;
    excludedPatch[1] = p1;
    excludedPatch[2] = NULL;
    excludedPatch[3] = NULL;
}

inline
RayExclusionSet::RayExclusionSet(const Patch *p0, const Patch *p1, const Patch *p2):
    excludedPatch(),
    excludedGeometry1(NULL),
    excludedGeometry2(NULL)
{
    excludedPatch[0] = p0;
    excludedPatch[1] = p1;
    excludedPatch[2] = p2;
    excludedPatch[3] = NULL;
}

inline
RayExclusionSet::RayExclusionSet(
    const Patch *p0,
    const Patch *p1,
    const Patch *p2,
    const Patch *p3,
    const Geometry *geometry1,
    const Geometry *geometry2):
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
RayExclusionSet::excludesGeometry(const Geometry *geometry) const {
    return geometry == excludedGeometry1 || geometry == excludedGeometry2;
}

#endif

#ifndef __RAY_BOUNDARY__
#define __RAY_BOUNDARY__

#include "vsdk/toolkit/common/VSDK.h"

#include "vsdk/toolkit/environment/geometry/element/RayInstancePath.h"
class Geometry;

// Mathematical provenance only. Scene resolves appearance from the borrowed leaf.
class RayBoundary {
public:
    double t;
    const Geometry* geometry;
    BOOLEAN normalReversed;
    // Face/element within the leaf geometry, or -1 when not applicable.
    long surfaceIndex;
private:
    // Ordered from the leaf instance outward. Values are snapshots, identities borrowed.
    RayInstancePath path;
public:
    RayInstancePath& getProvenance();
    const RayInstancePath& getProvenance() const;
    BOOLEAN appendInstance(const RayInstanceStep& step);
    long getInstanceCount() const;
    BOOLEAN getInstance(long index, RayInstanceStep& result) const;
    void clearInstances();

    RayBoundary();
    RayBoundary(const RayBoundary& other);
    RayBoundary& operator=(const RayBoundary& other);
    ~RayBoundary();
    RayBoundary(double t, const Geometry* geometry, BOOLEAN normalReversed = FALSE,
        long surfaceIndex = -1);
};

#endif

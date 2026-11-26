#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"

// Definitions needed by C++11 when the constants are bound to references
// (i.e. by the comparisons of the tests)
constexpr int Geometry::INSIDE;
constexpr int Geometry::LIMIT;
constexpr int Geometry::OUTSIDE;

void Geometry::doExtraInformation(const Ray&, double, RayHit*) {}
int Geometry::computeQuantitativeInvisibility(const Vector3Dd&, const Vector3Dd&) { return 0; }
int Geometry::doContainmentTest(const Vector3Dd&, double) { return OUTSIDE; }


RaySolidQueryStatus Geometry::classifyRay(const Ray&, const RayCrossingQuery&,
    RaySolidClassification& result) const
{
    result.clear();
    return RaySolidQueryStatus::Unsupported;
}

void Geometry::getBounds(GeometryBounds& result) const { result.setUnsupported(); }

RayFirstHitStatus Geometry::queryFirstHit(const Ray&, const RayCrossingQuery&, RayHit* result,
    RayBoundary* boundary) const
{
    if (result) result->clear();
    if (boundary) *boundary = RayBoundary();
    return RayFirstHitStatus::Unsupported;
}

RayFirstHitStatus Geometry::querySurfaceDetail(const Ray&, const RayBoundary&, RayHit* result) const
{
    if (result) result->clear();
    return RayFirstHitStatus::Unsupported;
}

GeometryContainment Geometry::classifyPointCertified(const Vector3Dd&, double) const
{
    return GeometryContainment::Unsupported;
}

GeometryContainment Geometry::classifyBoxCertified(const Vector3Dd&, const Vector3Dd&) const
{
    return GeometryContainment::Unsupported;
}

GeometryContainment Geometry::classifyPoint(const Vector3Dd&, double) const
{ return GeometryContainment::Unsupported; }

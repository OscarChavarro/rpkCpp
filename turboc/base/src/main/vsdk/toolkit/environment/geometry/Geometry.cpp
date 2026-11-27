#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"

void Geometry::doExtraInformation(const Ray&, double, RayHit*) {}
int Geometry::computeQuantitativeInvisibility(const Vector3Dd&, const Vector3Dd&) { return 0; }
int Geometry::doContainmentTest(const Vector3Dd&, double) { return OUTSIDE; }

const Class Geometry::CLASS = { "Geometry", &Entity::CLASS };

const Class *
Geometry::getClass() const
{
    return &CLASS;
}

RaySolidQueryStatus::Value Geometry::classifyRay(const Ray&, const RayCrossingQuery&,
    RaySolidClassification& result) const
{
    result.clear();
    return RaySolidQueryStatus::Unsupported;
}

void Geometry::getBounds(GeometryBounds& result) const { result.setUnsupported(); }

RayFirstHitStatus::Value Geometry::queryFirstHit(const Ray&, const RayCrossingQuery&, RayHit* result,
    RayBoundary* boundary) const
{
    if (result) result->clear();
    if (boundary) *boundary = RayBoundary();
    return RayFirstHitStatus::Unsupported;
}

RayFirstHitStatus::Value Geometry::querySurfaceDetail(const Ray&, const RayBoundary&, RayHit* result) const
{
    if (result) result->clear();
    return RayFirstHitStatus::Unsupported;
}

GeometryContainment::Value Geometry::classifyPointCertified(const Vector3Dd&, double) const
{
    return GeometryContainment::Unsupported;
}

GeometryContainment::Value Geometry::classifyBoxCertified(const Vector3Dd&, const Vector3Dd&) const
{
    return GeometryContainment::Unsupported;
}

GeometryContainment::Value Geometry::classifyPoint(const Vector3Dd&, double) const
{ return GeometryContainment::Unsupported; }

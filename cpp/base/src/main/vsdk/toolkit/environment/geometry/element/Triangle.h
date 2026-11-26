#ifndef __TRIANGLE__
#define __TRIANGLE__

#include "java/lang/String.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Intersection.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "vsdk/toolkit/environment/geometry/element/RayFirstHitStatus.h"

class Ray;

class Triangle
{
  private:
    int point0;
    int point1;
    int point2;
    Vector3Dd normal;

    static java::String intToStr(int val);
    static RayFirstHitStatus intersectRayCore(const Ray& ray, const Vector3Dd& v0,
        const Vector3Dd& v1, const Vector3Dd& v2, double minT, double maxT,
        double determinantTolerance, bool includeMin, bool checkArithmetic,
        double& t, double& u, double& v);

  public:
    enum class ProjectionAxis { X = 0, Y = 1, Z = 2 };
    Triangle();
    Triangle(int point0, int point1, int point2);

    int getPoint0() const;
    int getPoint1() const;
    int getPoint2() const;

    void setPoint0(int value);
    void setPoint1(int value);
    void setPoint2(int value);
    const Vector3Dd& getNormal() const;
    void setNormal(const Vector3Dd& normal);

    static Intersection* doIntersectionWithTriangle(
        const Ray& ray,
        const Vector3Dd& v0,
        const Vector3Dd& v1,
        const Vector3Dd& v2);
    // Shared allocation-free Möller–Trumbore query; returns affine t and the
    // barycentric coordinates corresponding to v1 and v2.
    static bool intersectRay(const Ray& ray, const Vector3Dd& v0,
        const Vector3Dd& v1, const Vector3Dd& v2, double& t, double& u, double& v);
    static RayFirstHitStatus queryIntersection(const Ray& ray, const Vector3Dd& v0,
        const Vector3Dd& v1, const Vector3Dd& v2, const RayCrossingQuery& query,
        double& t, double& u, double& v);
    static bool supportingPlane(const Vector3Dd& p0, const Vector3Dd& p1,
        const Vector3Dd& p2, double minimumNormalLength, Vector3Dd& normal, double& distance);
    static ProjectionAxis dominantProjection(const Vector3Dd& normal);
    static bool requiresProjectionSwap(const Vector3Dd& p0, const Vector3Dd& p1,
        const Vector3Dd& p2, ProjectionAxis axis);
    // Apply the optional vertex swap before testing projected containment.
    static bool containsProjected(const Vector3Dd& p0, const Vector3Dd& p1,
        const Vector3Dd& p2, const Vector3Dd& point, ProjectionAxis axis);
    static int containmentTest(
        const Vector3Dd& p0,
        const Vector3Dd& p1,
        const Vector3Dd& p2,
        const Vector3Dd& p,
        double distanceTolerance);
    static void minMax(
        const Vector3Dd& p0,
        const Vector3Dd& p1,
        const Vector3Dd& p2,
        double mm[6]);

    /**
    Provides an object to text report convertion, optimized for human
    readability and debugging. Do not use this method for serialization
    or persistence purposes.
    @return human readable representation of current triangle
    */
    java::String toString() const;
};

inline bool Triangle::supportingPlane(const Vector3Dd& p0, const Vector3Dd& p1,
    const Vector3Dd& p2, double minimumNormalLength, Vector3Dd& normal, double& distance)
{
    normal = p0.subtract(p1).crossProduct(p2.subtract(p1));
    const double length = normal.length();
    if (!java::Double::isFinite(length) || length == 0 || length < minimumNormalLength) return false;
    normal = normal.multiply(1.0 / length);
    distance = -normal.dotProduct(p0);
    return true;
}

inline Triangle::ProjectionAxis Triangle::dominantProjection(const Vector3Dd& normal)
{
    const double x = java::Math::abs(normal.x()), y = java::Math::abs(normal.y());
    const double z = java::Math::abs(normal.z());
    return x > y ? (x > z ? ProjectionAxis::X : ProjectionAxis::Z) :
        (y > z ? ProjectionAxis::Y : ProjectionAxis::Z);
}

inline bool Triangle::requiresProjectionSwap(const Vector3Dd& p0, const Vector3Dd& p1,
    const Vector3Dd& p2, ProjectionAxis axis)
{
    if (axis == ProjectionAxis::X)
        return (p1.y()-p2.y())*(p1.z()-p0.z()) < (p1.z()-p2.z())*(p1.y()-p0.y());
    if (axis == ProjectionAxis::Y)
        return (p1.x()-p2.x())*(p1.z()-p0.z()) < (p1.z()-p2.z())*(p1.x()-p0.x());
    return (p1.x()-p2.x())*(p1.y()-p0.y()) < (p1.y()-p2.y())*(p1.x()-p0.x());
}

inline bool Triangle::containsProjected(const Vector3Dd& p0, const Vector3Dd& p1,
    const Vector3Dd& p2, const Vector3Dd& point, ProjectionAxis axis)
{
    double a0, a1, a2, b0, b1, b2, s, t;
    if (axis == ProjectionAxis::X) {
        a0=p0.y(); a1=p1.y(); a2=p2.y(); b0=p0.z(); b1=p1.z(); b2=p2.z(); s=point.y(); t=point.z();
    } else if (axis == ProjectionAxis::Y) {
        a0=p0.x(); a1=p1.x(); a2=p2.x(); b0=p0.z(); b1=p1.z(); b2=p2.z(); s=point.x(); t=point.z();
    } else {
        a0=p0.x(); a1=p1.x(); a2=p2.x(); b0=p0.y(); b1=p1.y(); b2=p2.y(); s=point.x(); t=point.y();
    }
    if ((a1-s)*(b1-b0) < (b1-t)*(a1-a0)) return false;
    if ((a2-s)*(b2-b1) < (b2-t)*(a2-a1)) return false;
    if ((a0-s)*(b0-b2) < (b0-t)*(a0-a2)) return false;
    return true;
}

inline bool Triangle::intersectRay(const Ray& ray, const Vector3Dd& v0,
    const Vector3Dd& v1, const Vector3Dd& v2, double& t, double& u, double& v)
{
    return intersectRayCore(ray, v0, v1, v2, VSDK::EPSILON,
        java::Double::POSITIVE_INFINITY, VSDK::EPSILON, false, false, t, u, v) == RayFirstHitStatus::Hit;
}

inline RayFirstHitStatus Triangle::intersectRayCore(const Ray& ray, const Vector3Dd& v0,
    const Vector3Dd& v1, const Vector3Dd& v2, double minT, double maxT,
    double determinantTolerance, bool includeMin, bool checkArithmetic,
    double& t, double& u, double& v)
{
    const Vector3Dd e1 = v1.subtract(v0);
    const Vector3Dd e2 = v2.subtract(v0);
    const Vector3Dd h = ray.getDirection().crossProduct(e2);
    const double determinant = e1.dotProduct(h);
    if (checkArithmetic && (!java::Double::isFinite(e1.x()) || !java::Double::isFinite(e1.y()) ||
        !java::Double::isFinite(e1.z()) || !java::Double::isFinite(e2.x()) ||
        !java::Double::isFinite(e2.y()) || !java::Double::isFinite(e2.z()) ||
        !java::Double::isFinite(determinant))) return RayFirstHitStatus::Indeterminate;
    Vector3Dd geometricNormal;
    if (checkArithmetic) {
        geometricNormal = e1.crossProduct(e2);
        const double squared = geometricNormal.dotProduct(geometricNormal);
        if (!java::Double::isFinite(squared)) return RayFirstHitStatus::Indeterminate;
        if (squared == 0) return geometricNormal.x() == 0 && geometricNormal.y() == 0 && geometricNormal.z() == 0 ?
            RayFirstHitStatus::Miss : RayFirstHitStatus::Indeterminate;
    }
    if (checkArithmetic && determinant == 0) {
        const Vector3Dd& normal = geometricNormal;
        const Vector3Dd& direction = ray.getDirection();
        const double normalDotDirection = normal.dotProduct(direction);
        if (!java::Double::isFinite(normalDotDirection) || normalDotDirection != 0 ||
            (normal.x() != 0 && direction.x() != 0 && normal.x()*direction.x() == 0) ||
            (normal.y() != 0 && direction.y() != 0 && normal.y()*direction.y() == 0) ||
            (normal.z() != 0 && direction.z() != 0 && normal.z()*direction.z() == 0))
            return RayFirstHitStatus::Indeterminate;
        const double side = ray.getOrigin().subtract(v0).dotProduct(normal);
        if (!java::Double::isFinite(side)) return RayFirstHitStatus::Indeterminate;
        return side == 0 ? RayFirstHitStatus::Indeterminate : RayFirstHitStatus::Miss;
    }
    if (java::Math::abs(determinant) < determinantTolerance) return RayFirstHitStatus::Miss;
    const double inverse = 1.0 / determinant;
    const Vector3Dd offset = ray.getOrigin().subtract(v0);
    u = inverse * offset.dotProduct(h);
    if (checkArithmetic && !java::Double::isFinite(u)) return RayFirstHitStatus::Indeterminate;
    if (u < 0.0 || u > 1.0) return RayFirstHitStatus::Miss;
    const Vector3Dd q = offset.crossProduct(e1);
    v = inverse * ray.getDirection().dotProduct(q);
    if (checkArithmetic && !java::Double::isFinite(v)) return RayFirstHitStatus::Indeterminate;
    if (v < 0.0 || u + v > 1.0) return RayFirstHitStatus::Miss;
    t = inverse * e2.dotProduct(q);
    if (checkArithmetic && !java::Double::isFinite(t)) return RayFirstHitStatus::Indeterminate;
    return (includeMin ? t >= minT : t > minT) && t <= maxT ?
        RayFirstHitStatus::Hit : RayFirstHitStatus::Miss;
}

inline Triangle::Triangle() : point0(0), point1(0), point2(0), normal(0, 0, 0)
{
}

inline Triangle::Triangle(int point0, int point1, int point2)
    : point0(point0), point1(point1), point2(point2), normal(0, 0, 0)
{
}

inline int Triangle::getPoint0() const
{
    return point0;
}

inline int Triangle::getPoint1() const
{
    return point1;
}

inline int Triangle::getPoint2() const
{
    return point2;
}

inline void Triangle::setPoint0(int value)
{
    point0 = value;
}

inline void Triangle::setPoint1(int value)
{
    point1 = value;
}

inline void Triangle::setPoint2(int value)
{
    point2 = value;
}

inline const Vector3Dd& Triangle::getNormal() const
{
    return normal;
}

inline void Triangle::setNormal(const Vector3Dd& inNormal)
{
    normal = inNormal;
}

#endif

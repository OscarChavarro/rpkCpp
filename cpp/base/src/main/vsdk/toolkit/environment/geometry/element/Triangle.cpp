#include <cmath>
#include <cstdio>

#include "java/lang/Math.h"
#include "java/lang/Double.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/processing/Containment.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"

static Vector3Dd closestPointOnTriangle(const Vector3Dd& p, const Vector3Dd& a, const Vector3Dd& b, const Vector3Dd& c)
{
    Vector3Dd ab = b.subtract(a), ac = c.subtract(a), ap = p.subtract(a);
    double d1 = ab.dotProduct(ap), d2 = ac.dotProduct(ap);
    if (d1 <= 0 && d2 <= 0) return a;
    Vector3Dd bp = p.subtract(b);
    double d3 = ab.dotProduct(bp), d4 = ac.dotProduct(bp);
    if (d3 >= 0 && d4 <= d3) return b;
    double vc = d1*d4 - d3*d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) return a.add(ab.multiply(d1/(d1-d3)));
    Vector3Dd cp = p.subtract(c);
    double d5 = ab.dotProduct(cp), d6 = ac.dotProduct(cp);
    if (d6 >= 0 && d5 <= d6) return c;
    double vb = d5*d2 - d1*d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) return a.add(ac.multiply(d2/(d2-d6)));
    double va = d3*d6 - d5*d4;
    if (va <= 0 && (d4-d3) >= 0 && (d5-d6) >= 0) return b.add(c.subtract(b).multiply((d4-d3)/((d4-d3)+(d5-d6))));
    double denom = 1.0/(va+vb+vc);
    double v = vb*denom, w = vc*denom;
    return a.add(ab.multiply(v)).add(ac.multiply(w));
}

RayFirstHitStatus Triangle::queryIntersection(const Ray& ray, const Vector3Dd& v0,
    const Vector3Dd& v1, const Vector3Dd& v2, const RayCrossingQuery& query,
    double& t, double& u, double& v)
{
    const Vector3Dd& p = ray.getOrigin();
    const Vector3Dd& d = ray.getDirection();
    if (java::Double::isNaN(query.minT) || java::Double::isNaN(query.maxT) || query.minT > query.maxT ||
        !java::Double::isFinite(query.minimumChordSquared) || query.minimumChordSquared < 0 ||
        java::Double::isNaN(query.minimumExteriorApproach) ||
        !java::Double::isFinite(p.x()) || !java::Double::isFinite(p.y()) || !java::Double::isFinite(p.z()) ||
        !java::Double::isFinite(d.x()) || !java::Double::isFinite(d.y()) || !java::Double::isFinite(d.z()) ||
        (d.x() == 0 && d.y() == 0 && d.z() == 0) ||
        !java::Double::isFinite(v0.x()) || !java::Double::isFinite(v0.y()) || !java::Double::isFinite(v0.z()) ||
        !java::Double::isFinite(v1.x()) || !java::Double::isFinite(v1.y()) || !java::Double::isFinite(v1.z()) ||
        !java::Double::isFinite(v2.x()) || !java::Double::isFinite(v2.y()) || !java::Double::isFinite(v2.z()))
        return RayFirstHitStatus::InvalidInput;
    if (query.minimumChordSquared != 0 || query.minimumExteriorApproach != java::Double::NEGATIVE_INFINITY)
        return RayFirstHitStatus::Unsupported;
    const double lengthSquared = d.dotProduct(d);
    if (!java::Double::isFinite(lengthSquared) || lengthSquared == 0) return RayFirstHitStatus::Indeterminate;
    return intersectRayCore(ray, v0, v1, v2, query.minT, query.maxT, 0, true, true, t, u, v);
}

Intersection* Triangle::doIntersectionWithTriangle(const Ray& ray, const Vector3Dd& v0, const Vector3Dd& v1, const Vector3Dd& v2)
{
    double t, u, v;
    if (!intersectRay(ray, v0, v1, v2, t, u, v)) return 0;

    Vector3Dd point = ray.getOrigin().add(ray.getDirection().multiply(t));
    Vector3Dd normal = v1.subtract(v0).crossProduct(v2.subtract(v0)).normalized();
    return new Intersection(t, point, normal);
}

int Triangle::containmentTest(const Vector3Dd& p0, const Vector3Dd& p1, const Vector3Dd& p2, const Vector3Dd& p, double distanceTolerance)
{
    Vector3Dd q = closestPointOnTriangle(p, p0, p1, p2);
    if (q.subtract(p).length() <= distanceTolerance) return static_cast<int>(Containment::LIMIT);
    return static_cast<int>(Containment::OUTSIDE);
}

void Triangle::minMax(const Vector3Dd& p0, const Vector3Dd& p1, const Vector3Dd& p2, double mm[6])
{
    mm[0] = java::Math::min(p0.x(), java::Math::min(p1.x(), p2.x()));
    mm[1] = java::Math::min(p0.y(), java::Math::min(p1.y(), p2.y()));
    mm[2] = java::Math::min(p0.z(), java::Math::min(p1.z(), p2.z()));
    mm[3] = java::Math::max(p0.x(), java::Math::max(p1.x(), p2.x()));
    mm[4] = java::Math::max(p0.y(), java::Math::max(p1.y(), p2.y()));
    mm[5] = java::Math::max(p0.z(), java::Math::max(p1.z(), p2.z()));
}

java::String Triangle::intToStr(int val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", val);
    return java::String(buf);
}

java::String Triangle::toString() const
{
    return java::String("f < ") + intToStr(point0) + ", " +
        intToStr(point1) + ", " +
        intToStr(point2) + " >";
}

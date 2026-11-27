#include <math.h>
#include <stdio.h>

#include "java/lang/Math.h"
#include "java/lang/Double.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/processing/Containment.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"
#include "vsdk/toolkit/common/Cpp98Support.h"

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

RayFirstHitStatus::Value Triangle::queryIntersection(const Ray& ray, const Vector3Dd& v0,
    const Vector3Dd& v1, const Vector3Dd& v2, const RayCrossingQuery& query,
    double& t, double& u, double& v)
{
    const Vector3Dd& p = ray.getOrigin();
    const Vector3Dd& d = ray.getDirection();
    if (Double::isNaN(query.minT) || Double::isNaN(query.maxT) || query.minT > query.maxT ||
        !Double::isFinite(query.minimumChordSquared) || query.minimumChordSquared < 0 ||
        Double::isNaN(query.minimumExteriorApproach) ||
        !Double::isFinite(p.x()) || !Double::isFinite(p.y()) || !Double::isFinite(p.z()) ||
        !Double::isFinite(d.x()) || !Double::isFinite(d.y()) || !Double::isFinite(d.z()) ||
        (d.x() == 0 && d.y() == 0 && d.z() == 0) ||
        !Double::isFinite(v0.x()) || !Double::isFinite(v0.y()) || !Double::isFinite(v0.z()) ||
        !Double::isFinite(v1.x()) || !Double::isFinite(v1.y()) || !Double::isFinite(v1.z()) ||
        !Double::isFinite(v2.x()) || !Double::isFinite(v2.y()) || !Double::isFinite(v2.z()))
        return RayFirstHitStatus::InvalidInput;
    if (query.minimumChordSquared != 0 || query.minimumExteriorApproach != Double::NEGATIVE_INFINITY)
        return RayFirstHitStatus::Unsupported;
    const double lengthSquared = d.dotProduct(d);
    if (!Double::isFinite(lengthSquared) || lengthSquared == 0) return RayFirstHitStatus::Indeterminate;
    return intersectRayCore(ray, v0, v1, v2, query.minT, query.maxT, 0, TRUE, TRUE, t, u, v);
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
    if (q.subtract(p).length() <= distanceTolerance) return ((int)(Containment::LIMIT));
    return ((int)(Containment::OUTSIDE));
}

void Triangle::minMax(const Vector3Dd& p0, const Vector3Dd& p1, const Vector3Dd& p2, double mm[6])
{
    mm[0] = Math::min(p0.x(), Math::min(p1.x(), p2.x()));
    mm[1] = Math::min(p0.y(), Math::min(p1.y(), p2.y()));
    mm[2] = Math::min(p0.z(), Math::min(p1.z(), p2.z()));
    mm[3] = Math::max(p0.x(), Math::max(p1.x(), p2.x()));
    mm[4] = Math::max(p0.y(), Math::max(p1.y(), p2.y()));
    mm[5] = Math::max(p0.z(), Math::max(p1.z(), p2.z()));
}

String Triangle::intToStr(int val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", val);
    return String(buf);
}

String Triangle::toString() const
{
    return String("f < ") + intToStr(point0) + ", " +
        intToStr(point1) + ", " +
        intToStr(point2) + " >";
}

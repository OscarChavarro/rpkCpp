#include <cmath>

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/processing/ComputationalGeometry.h"
#include "vsdk/toolkit/processing/Containment.h"
double ComputationalGeometry::lineToPointDistance(
    const Vector3Dd& p0,
    const Vector3Dd& p1,
    const Vector3Dd& p)
{
    Vector3Dd lineVector = p1.subtract(p0);
    double denominator = lineVector.length();
    if ( denominator < VSDK::EPSILON ) {
        return NAN;
    }

    Vector3Dd v = p1.subtract(p0);
    Vector3Dd w = p.subtract(p0);
    double c1 = w.dotProduct(v);
    if ( c1 <= 0.0 ) {
        return p.subtract(p0).length();
    }

    double c2 = v.dotProduct(v);
    if ( c2 <= c1 ) {
        return p.subtract(p1).length();
    }

    double b = c1 / c2;
    Vector3Dd pb = p0.add(v.multiply(b));
    return p.subtract(pb).length();
}

int ComputationalGeometry::lineContainmentTest(
    const Vector3Dd& p0,
    const Vector3Dd& p1,
    const Vector3Dd& p,
    double distanceTolerance)
{
    double d = lineToPointDistance(p0, p1, p);
    if ( d <= distanceTolerance ) {
        return static_cast<int>(Containment::LIMIT);
    }
    return static_cast<int>(Containment::OUTSIDE);
}

int ComputationalGeometry::lineSegmentContainmentTest(
    const Vector3Dd& p0,
    const Vector3Dd& p1,
    const Vector3Dd& p,
    double distanceTolerance)
{
    Vector3Dd a = p1.subtract(p0);
    Vector3Dd b = p.subtract(p0);
    double denominator = a.length();
    if ( denominator < VSDK::EPSILON ) {
        return static_cast<int>(Containment::OUTSIDE);
    }

    double numerator = a.crossProduct(b).length();
    double d = numerator / denominator;
    if ( d <= distanceTolerance ) {
        double t = a.dotProduct(b) / a.dotProduct(a);
        if ( t < -VSDK::EPSILON || t > 1 + VSDK::EPSILON ) {
            return static_cast<int>(Containment::OUTSIDE);
        }
        return static_cast<int>(Containment::LIMIT);
    }
    return static_cast<int>(Containment::OUTSIDE);
}

#ifndef __INFINITE_PLANE__
#define __INFINITE_PLANE__

#include "vsdk/toolkit/common/VSDK.h"

#include "java/lang/Double.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/environment/geometry/surface/HalfSpace.h"
class QuadMesh;
class Ray;
class RayHit;

class InfinitePlane : public HalfSpace {
private:
    // Plane equation ax + by + cz + d = 0.
    double a;
    double b;
    double c;
    double d;
    RayFirstHitStatus::Value evaluateRay(const Ray& ray, const RayCrossingQuery& query,
        double& depth) const;
    Vector3Dd surfaceNormal() const;

public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    InfinitePlane(const InfinitePlane& other);
    InfinitePlane(double a, double b, double c, double d);
    InfinitePlane(const Vector3Dd& normal, const Vector3Dd& pointInPlane);
    InfinitePlane(const Vector3Dd& p0, const Vector3Dd& p1, const Vector3Dd& p2);

    void clone(const InfinitePlane& other);
    virtual Geometry* copyGeometry() const { return new InfinitePlane(*this); }
    virtual void getBounds(GeometryBounds& result) const;
    virtual RayCrossingSupport::Value emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
        RayCrossingSink& sink) const;
    virtual RayFirstHitStatus::Value querySurfaceDetail(const Ray& ray, const RayBoundary& boundary, RayHit* hit) const;
    virtual RayFirstHitStatus::Value queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
        RayHit* result, RayBoundary* boundary = NULL) const;

    // Intersect a plane from its precomputed signed numerator and directional
    // denominator. The caller supplies the parallel and interval policies.
    static RayFirstHitStatus::Value intersectionDepth(double numerator, double denominator,
        double parallelTolerance, double minT, double maxT, BOOLEAN checkArithmetic,
        double& depth);

    Ray* doIntersectionFirstHit(const Ray& inout_rayo);
    virtual BOOLEAN doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);

    Ray* doIntersectionWithNegative(const Ray& inout_rayo);

    // Legacy predicates in unnormalized plane units (Java parity). The half-space
    // form classifies n.p + d <= 0; the surface form reports LIMIT on the plane
    // and OUTSIDE elsewhere (its -INSIDE equals OUTSIDE).
    int doContainmentTestHalfSpace(const Vector3Dd& p, double distanceTolerance);
    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance);

    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData);
    virtual double* getMinMax();

    // Raw coefficients preserve the equation scale; getNormal() is normalized.
    Vector3Dd getCoefficients() const { return Vector3Dd(a, b, c); }
    double getConstantCoefficient() const { return d; }
    void setCoefficients(const Vector3Dd& normal, double constant) {
        a = normal.x(); b = normal.y(); c = normal.z(); d = constant;
    }
    Vector3Dd getNormal() const;
    double getD() const;

    void setNormal(const Vector3Dd& n);
    void setD(double d);
    void setFromPointNormal(const Vector3Dd& p, const Vector3Dd& n);

    double pointDistance(const Vector3Dd& p) const;
    Vector3Dd projectPoint(const Vector3Dd& p) const;
    Vector3Dd mirrorPoint(const Vector3Dd& p) const;

    BOOLEAN overlapsWith(const InfinitePlane& other, double tolerance) const;
    String toString() const;

    double getA() const;
    void setA(double a);
    double getB() const;
    void setB(double b);
    double getC() const;
    void setC(double c);

    /**
    A finite part of the plane, to present it: a square of side `size`
    divided into `tiles` x `tiles` cells, centered near a given point (i.e.
    the point of the plane viewed by a camera). The center is the projection
    of the point onto the plane, moved to the nearest corner of the cells of
    a grid fixed on the plane (with a corner at the point of the plane
    closest to the origin), so the lines of the square stay in the same
    places of the plane as the point moves. The sides follow two orthonormal
    directions (u, v) on the plane, with (u, v, normal) right handed: u is
    perpendicular to z (to x if the normal is almost along z).
    @param nearPoint point near which the square is centered
    @param size side of the square
    @param tiles cells along each side of the square, at least 1
    @return a mesh of `tiles` x `tiles` quadrilaterals, owned by the caller,
    or null if the plane is degenerated (zero normal)
    */
    QuadMesh* exportToQuadMesh(const Vector3Dd& nearPoint, double size,
        int tiles) const;
};

inline RayFirstHitStatus::Value InfinitePlane::intersectionDepth(double numerator, double denominator,
    double parallelTolerance, double minT, double maxT, BOOLEAN checkArithmetic, double& depth)
{
    if (checkArithmetic && (!Double::isFinite(numerator) ||
        !Double::isFinite(denominator) || !Double::isFinite(parallelTolerance) ||
        parallelTolerance < 0 || Double::isNaN(minT) || Double::isNaN(maxT) || minT > maxT))
        return RayFirstHitStatus::InvalidInput;
    const BOOLEAN parallel = parallelTolerance == 0 ? denominator == 0 :
        denominator < parallelTolerance && denominator > -parallelTolerance;
    if (parallel) return checkArithmetic && numerator == 0 ?
        RayFirstHitStatus::Indeterminate : RayFirstHitStatus::Miss;
    depth = numerator / denominator;
    if (checkArithmetic && !Double::isFinite(depth)) return RayFirstHitStatus::Indeterminate;
    return depth >= minT && depth <= maxT ? RayFirstHitStatus::Hit : RayFirstHitStatus::Miss;
}

#endif

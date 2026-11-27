#include <stddef.h>
#include "java/lang/Double.h"
#include "java/lang/IllegalStateException.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include <math.h>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "vsdk/toolkit/common/statistics/RaytraceStatistics.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#ifndef __TURBOC__
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#endif

Sphere::Sphere(double r) : radius(r), radiusSquared(r * r) {
    // Turbo C++ 3.0 crashes with the new of a template class as an argument
    MemberEntityControlAccessor<Sphere, double> *radiusAccessor =
        new MemberEntityControlAccessor<Sphere, double>(
            &Sphere::getRadius, &Sphere::setRadius);
    addControlSpecification("double;radius;(0, INFINITE)", radiusAccessor);
}

Ray*
Sphere::doIntersectionFirstHit(const Ray& inoutRay) {
    double nearT, farT;
    if (intersectionRoots(inoutRay.getOrigin(), inoutRay.getDirection(), radius,
            RayCrossingQuery(0, Double::POSITIVE_INFINITY), nearT, farT) == 0) {
        return NULL;
    }
    return new Ray(inoutRay.withT(nearT));
}

BOOLEAN
Sphere::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) {
    return doIntersectionFirstHit(inRay,
        RayCrossingQuery(0, Double::POSITIVE_INFINITY), outHit);
}

BOOLEAN
Sphere::doIntersectionFirstHit(const Ray& inRay, const RayCrossingQuery& query,
    RayHit* outHit) {
    double t, farT;
    if (intersectionRoots(inRay.getOrigin(), inRay.getDirection(), radius,
            query, t, farT) == 0) {
        return FALSE;
    }

    if (outHit != NULL) {
        if (outHit->shouldStoreRay() || outHit->needsAnySurfaceData()) {
            Ray hitRay = inRay.withT(t);
            outHit->setRay(hitRay);
            if (outHit->needsAnySurfaceData()) {
                doExtraInformation(hitRay, t, outHit);
                outHit->setRay(hitRay);
            }
        }
        else {
            outHit->setHitDistance(t);
        }
    }
    return TRUE;
}

void
Sphere::doExtraInformation(const Ray& inRay, double inT, RayHit* outData) {
    fillSurfaceDetail(inRay, inT, outData);
}

void
Sphere::fillSurfaceDetail(const Ray& inRay, double inT, RayHit* outData) const {
    if (outData == NULL) {
        return;
    }

    outData->beginDetailEvaluation();
    RaytraceStatistics::recordGeometryDetailComputation();
    BOOLEAN needsNormalVector = outData->needsNormal() || outData->needsTextureCoordinates() || outData->needsTangent();
    if (!outData->needsPoint() && !needsNormalVector) {
        return;
    }

    Vector3Dd point(
        inRay.getOrigin().x() + inT * inRay.getDirection().x(),
        inRay.getOrigin().y() + inT * inRay.getDirection().y(),
        inRay.getOrigin().z() + inT * inRay.getDirection().z());

    if (outData->needsPoint()) {
        outData->point = point;
        if (Double::isFinite(point.x()) && Double::isFinite(point.y()) && Double::isFinite(point.z()))
            outData->markDetailAvailable(RayHit::DETAIL_POINT);
    }

    Vector3Dd normal;
    if (needsNormalVector) {
        const double pointSquared = point.dotProduct(point);
        // Vector3Dd::normalized intentionally treats lengths below VSDK::EPSILON
        // as zero; a small but valid sphere still needs a unit surface normal.
        if (Double::isFinite(pointSquared) && pointSquared > 0 && pointSquared < VSDK::EPSILON)
            normal = point.multiply(1.0 / Math::sqrt(pointSquared));
        else
            normal = point.normalized();
        if (outData->needsNormal()) {
            outData->normal = normal;
            const double normalSquared = normal.dotProduct(normal);
            if (Double::isFinite(normalSquared) && normalSquared > 0 &&
                Double::isFinite(normal.x()) && Double::isFinite(normal.y()) && Double::isFinite(normal.z()))
                outData->markDetailAvailable(RayHit::DETAIL_NORMAL);
        }
    }

    if (!outData->needsTextureCoordinates() && !outData->needsTangent()) {
        return;
    }

    double theta;
    double phi = acos(normal.z());

    if (normal.x() > VSDK::EPSILON) {
        theta = atan(normal.y() / normal.x()) + 3 * M_PI / 2;
    }
    else if (normal.x() < VSDK::EPSILON) {
        theta = atan(normal.y() / normal.x()) + 3 * M_PI / 2;
        theta += M_PI;
        if (theta > 2 * M_PI) {
            theta -= 2 * M_PI;
        }
    }
    else {
        theta = 0.0;
    }

    if (outData->needsTextureCoordinates()) {
        outData->u = ((theta + M_PI / 2) / (2 * M_PI));
        outData->v = 1 - (phi / M_PI);
        if (Double::isFinite(outData->u) && Double::isFinite(outData->v))
            outData->markDetailAvailable(RayHit::DETAIL_UV);
    }
    if (outData->needsTangent()) {
        outData->tangent = Vector3Dd(
            sin(theta - M_PI / 2),
            -cos(theta - M_PI / 2),
            0);
        if (Double::isFinite(outData->tangent.x()) && Double::isFinite(outData->tangent.y()))
            outData->markDetailAvailable(RayHit::DETAIL_TANGENT);
    }
}

GeometryContainment::Value
Sphere::classifyBoxCertified(const Vector3Dd& center, const Vector3Dd& halfWidths) const {
    if (!Double::isFinite(radius) || radius <= 0) return GeometryContainment::InvalidInput;
    if (!FloatingInterval::isSupportedEnvironment()) return GeometryContainment::Unsupported;
    const double c[3] = {center.x(), center.y(), center.z()};
    const double h[3] = {halfWidths.x(), halfWidths.y(), halfWidths.z()};
    // Squared distances from the centre to the farthest and nearest box points.
    FloatingInterval farthest = FloatingInterval::exact(0), nearest = FloatingInterval::exact(0);
    for (int i = 0; i < 3; ++i) {
        const FloatingInterval range = FloatingInterval::around(c[i], h[i]);
        if (!range.isValid()) return GeometryContainment::InvalidInput;
        const double extreme = Math::max(Math::abs(range.getLower()), Math::abs(range.getUpper()));
        farthest = farthest.add(FloatingInterval::exact(extreme).square());
        if (range.getLower() > 0) nearest = nearest.add(FloatingInterval::exact(range.getLower()).square());
        else if (range.getUpper() < 0) nearest = nearest.add(FloatingInterval::exact(range.getUpper()).square());
    }
    const FloatingInterval squaredRadius = FloatingInterval::exact(radius).square();
    if (!farthest.isValid() || !nearest.isValid() || !squaredRadius.isValid()) return GeometryContainment::Indeterminate;
    if (farthest.getUpper() < squaredRadius.getLower()) return GeometryContainment::Inside;
    if (nearest.getLower() > squaredRadius.getUpper()) return GeometryContainment::Outside;
    return GeometryContainment::Indeterminate;
}

GeometryContainment::Value
Sphere::classifyPointCertified(const Vector3Dd& p, double distanceTolerance) const {
    if (!Double::isFinite(radius) || radius <= 0 ||
        !Double::isFinite(distanceTolerance) || distanceTolerance < 0 ||
        !Double::isFinite(p.x()) || !Double::isFinite(p.y()) ||
        !Double::isFinite(p.z())) return GeometryContainment::InvalidInput;
    if (!FloatingInterval::isSupportedEnvironment()) return GeometryContainment::Unsupported;
    FloatingInterval squared=FloatingInterval::exact(0);
    squared=squared.add(FloatingInterval::exact(p.x()).square());
    squared=squared.add(FloatingInterval::exact(p.y()).square());
    squared=squared.add(FloatingInterval::exact(p.z()).square());
    const FloatingInterval signedDistance=squared.squareRoot().subtract(FloatingInterval::exact(radius));
    if (!signedDistance.isValid()) return GeometryContainment::Indeterminate;
    if (signedDistance.getUpper() < -distanceTolerance) return GeometryContainment::Inside;
    if (signedDistance.getLower() > distanceTolerance) return GeometryContainment::Outside;
    if (signedDistance.getLower() >= -distanceTolerance && signedDistance.getUpper() <= distanceTolerance)
        return GeometryContainment::Boundary;
    return GeometryContainment::Indeterminate;
}

GeometryContainment::Value
Sphere::classifyPoint(const Vector3Dd& p, double distanceTolerance) const {
    if (!Double::isFinite(radius) || radius <= 0 ||
        !Double::isFinite(distanceTolerance) || distanceTolerance < 0 ||
        !Double::isFinite(p.x()) || !Double::isFinite(p.y()) ||
        !Double::isFinite(p.z())) return GeometryContainment::InvalidInput;
    if (distanceTolerance>0) return classifyPointCertified(p, distanceTolerance);
    const double squared = p.dotProduct(p);
    const double radiusSquaredValue = radius * radius;
    if (!Double::isFinite(squared) || !Double::isFinite(radius + distanceTolerance) ||
        !Double::isFinite(radiusSquaredValue) || radiusSquaredValue == 0 ||
        (squared == 0 && (p.x() != 0 || p.y() != 0 || p.z() != 0)))
        return GeometryContainment::Indeterminate;
    const double distance = p.length();
    if (distance < radius - distanceTolerance) return GeometryContainment::Inside;
    if (distance > radius + distanceTolerance) return GeometryContainment::Outside;
    return GeometryContainment::Boundary;
}

int
Sphere::doContainmentTest(const Vector3Dd& p, double distanceTolerance) {
    const GeometryContainment::Value result = classifyPoint(p, distanceTolerance);
    if (result == GeometryContainment::Inside) return INSIDE;
    if (result == GeometryContainment::Boundary) return LIMIT;
    if (result == GeometryContainment::Outside) return OUTSIDE;
    // The legacy integer contract has no representation for uncertainty.
    // Do not silently convert an indeterminate or failed query to outside.
    throw IllegalStateException("Sphere containment cannot be represented by the legacy integer result");
}

double*
Sphere::getMinMax() {
    double* minmax = new double[6];
    int i;
    for (i = 0; i < 3; i++) {
        minmax[i] = -radius;
    }
    for (i = 3; i < 6; i++) {
        minmax[i] = radius;
    }
    return minmax;
}

double Sphere::getRadius() const { return radius; }
double Sphere::getRadiusSquared() const { return radiusSquared; }
void Sphere::setRadius(double value) { radius = value; radiusSquared = value * value; }

#ifndef __TURBOC__
Vector3Dd Sphere::spherePosition(double theta, double t, double r) {
    double phi = (t - 0.5) * M_PI;
    return Vector3Dd(
        cos(phi) * cos(theta) * r,
        cos(phi) * sin(theta) * r,
        sin(phi) * r);
}

PolyhedralBoundedSolid* Sphere::exportToPolyhedralBoundedSolid() {
    return buildPolyhedralBoundedSolid(DEFAULT_MERIDIANS, DEFAULT_PARALLELS, FALSE);
}

PolyhedralBoundedSolid* Sphere::exportToPolyhedralBoundedSolid(int meridians, int parallels,
    BOOLEAN withQuadrilaterals) {
    int normalizedMeridians = meridians < MIN_MERIDIANS ? MIN_MERIDIANS : meridians;
    int normalizedParallels = parallels < MIN_PARALLELS ? MIN_PARALLELS : parallels;

    if (!withQuadrilaterals && normalizedMeridians == DEFAULT_MERIDIANS &&
        normalizedParallels == DEFAULT_PARALLELS) {
        return exportToPolyhedralBoundedSolid();
    }

    return buildPolyhedralBoundedSolid(normalizedMeridians, normalizedParallels,
        withQuadrilaterals);
}

PolyhedralBoundedSolid* Sphere::buildPolyhedralBoundedSolid(int nmeridians, int nparalels,
    BOOLEAN withQuadrilaterals) {
    double theta;
    double phi;
    double dtheta = 2 * M_PI / ((double)nmeridians);
    double dphi = 1.0 / ((double)nparalels);
    int i, base2, base1;
    Vector3Dd pos;

    PolyhedralBoundedSolid* solid;

    solid = new PolyhedralBoundedSolid();
    pos = Vector3Dd(0, 0, -radius);
    PolyhedralBoundedSolidEulerOperators::mvfs(solid, pos, 1, 1);

    pos = spherePosition(dtheta, dphi, radius);
    PolyhedralBoundedSolidEulerOperators::smev(solid, 1, 1, 3, pos);
    pos = spherePosition(0, dphi, radius);
    PolyhedralBoundedSolidEulerOperators::smev(solid, 1, 3, 2, pos);

    PolyhedralBoundedSolidEulerOperators::mef(solid, 1, 1, 1, 3, 2, 3, 2);

    for (i = 2; i < nmeridians; i++) {
        theta = dtheta * ((double)i);
        pos = spherePosition(theta, dphi, radius);
        PolyhedralBoundedSolidEulerOperators::smev(solid, 1, 1, (i + 1) + 1, pos);
        PolyhedralBoundedSolidEulerOperators::mef(solid, 1, 1, (i + 0) + 1, (1), (i + 1) + 1, (1), i + 1);
    }

    PolyhedralBoundedSolidEulerOperators::mef(solid, 1, 1, (i + 1), (1), (2), (3), i + 1);
    base2 = i + 2;
    base1 = 2;

    int p;
    int nextFaceId = nmeridians + 2;
    for (p = 0; p < nparalels - 2; p++) {
        phi = ((double)(p + 2)) / ((double)nparalels);
        for (i = 0; i < nmeridians; i++) {
            theta = dtheta * ((double)i);
            pos = spherePosition(theta, phi, radius);
            PolyhedralBoundedSolidEulerOperators::smev(solid, 1, (i) + base1, (i) + base2, pos);
            if (i > 0) {
                int quadFaceId = nextFaceId++;
                PolyhedralBoundedSolidEulerOperators::mef(solid, 1, 1,
                    (i - 1) + base2,
                    (i - 1) + base1,
                    (i) + base2,
                    (i) + base1,
                    quadFaceId);
                if (!withQuadrilaterals) {
                    int diagFaceId = nextFaceId++;
                    PolyhedralBoundedSolidEulerOperators::smef(solid, quadFaceId,
                        (i - 1) + base2,
                        (i) + base1,
                        diagFaceId);
                }
            }
        }
        {
            int quadFaceId = nextFaceId++;
            PolyhedralBoundedSolidEulerOperators::mef(solid, 1, 1,
                (i + base2 - 1),
                (base1 + i - 1),
                (base2),
                (base2 + 1),
                quadFaceId);
            if (!withQuadrilaterals) {
                int diagFaceId = nextFaceId++;
                PolyhedralBoundedSolidEulerOperators::smef(solid, quadFaceId,
                    (i - 1) + base2,
                    base1,
                    diagFaceId);
            }
        }
        base1 = base2;
        base2 += nmeridians;
    }

    pos = Vector3Dd(0, 0, radius);
    PolyhedralBoundedSolidEulerOperators::smev(solid, 1, base1, base2, pos);

    for (i = 0; i < nmeridians - 2; i++) {
        PolyhedralBoundedSolidEulerOperators::mef(solid, 1, 1,
            base2,
            base1 + i,
            base1 + i + 1,
            base1 + i + 2,
            nextFaceId++);
    }

    PolyhedralBoundedSolidEulerOperators::mef(solid, 1, 1,
        base2,
        base1 + i,
        base1 + i + 1,
        base1,
        nextFaceId++);

    return solid;
}
#endif

Vector3Dd Sphere::spherePosition(double theta, double phi) {
    return Vector3Dd(
        cos(phi) * cos(theta) * radius,
        -cos(phi) * sin(theta) * radius,
        sin(phi) * radius);
}

Vector3Dd Sphere::sphereNormal(double theta, double phi) {
    return Vector3Dd(
        cos(phi) * cos(theta),
        -cos(phi) * sin(theta),
        sin(phi));
}

Vector3Dd Sphere::sphereTangent(double theta, double) {
    return Vector3Dd(
        sin(theta),
        cos(theta),
        0);
}

Vector3Dd Sphere::sphereBinormal(double theta, double phi) {
    return Vector3Dd(
        -sin(phi) * cos(theta),
        sin(phi) * sin(theta),
        cos(phi) * cos(theta) * cos(theta) +
        cos(phi) * sin(theta) * sin(theta));
}

const Class Sphere::CLASS = { "Sphere", &Solid::CLASS };

const Class *
Sphere::getClass() const
{
    return &CLASS;
}

// C++ port note: out of line (the cpp port defines it inline in the header)
int Sphere::intersectionRoots(const Vector3Dd& p, const Vector3Dd& d,
    double radius, const RayCrossingQuery& query, double& nearT, double& farT)
{
    const double lengthSquared = d.dotProduct(d);
    if (!(lengthSquared > 0.0) || query.minT > query.maxT) return 0;
    const double directionLength = sqrt(lengthSquared);
    const Vector3Dd unitDirection = d.multiply(1.0 / directionLength);
    const double radiusSquared = radius * radius;
    const double ocSquared = p.dotProduct(p);
    const BOOLEAN inside = ocSquared < radiusSquared;
    const double closest = -p.dotProduct(unitDirection);
    if (!inside && closest < query.minimumExteriorApproach) return 0;
    const double chordSquared = radiusSquared - ocSquared + closest * closest;
    if (chordSquared < query.minimumChordSquared) return 0;
    const double halfChord = sqrt(chordSquared);
    farT = (closest + halfChord) / directionLength;
    nearT = (closest - halfChord) / directionLength;
    const BOOLEAN nearValid = nearT >= query.minT && nearT <= query.maxT;
    const BOOLEAN farValid = farT >= query.minT && farT <= query.maxT;
    if (!nearValid && !farValid) return 0;
    if (!nearValid) nearT = farT;
    if (!farValid) farT = nearT;
    return nearT == farT ? 1 : 2;
}

RayCrossingSupport::Value Sphere::emitRayCrossings(const Ray& ray, const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    RayFirstHitStatus::Value failure;
    if (!validateSurfaceQuery(ray, query, failure)) {
        return failure == RayFirstHitStatus::InvalidInput ? RayCrossingSupport::InvalidInput
            : RayCrossingSupport::Indeterminate;
    }
    double nearT, farT;
    int count = intersectionRoots(ray.getOrigin(), ray.getDirection(), radius, query, nearT, farT);
    for (int i = 0; i < count; ++i) {
        double t = i == 0 ? nearT : farT;
        Vector3Dd point = ray.getOrigin().add(ray.getDirection().multiply(t));
        double orientation = point.dotProduct(ray.getDirection());
        RayCrossing::Kind kind = orientation < 0 ? RayCrossingKind::Entering : orientation > 0 ? RayCrossingKind::Leaving : RayCrossingKind::Tangent;
        sink.crossing(RayCrossing(t, this, kind));
    }
    return RayCrossingSupport::Supported;
}


RaySolidQueryStatus::Value Sphere::classifyRay(const Ray& ray,
    const RayCrossingQuery& query, RaySolidClassification& result) const
{
    result.clear();
    const Vector3Dd& origin = ray.getOrigin();
    const Vector3Dd& direction = ray.getDirection();
    const double infinity = Double::POSITIVE_INFINITY;
    // Surface compatibility cutoffs may discard genuine boundaries and cannot
    // define a solid classification. Applications apply those to surface queries.
    if (Double::isNaN(query.minT) || Double::isNaN(query.maxT) || query.minT >= query.maxT ||
        query.minimumChordSquared != 0.0 || query.minimumExteriorApproach != -infinity ||
        !Double::isFinite(radius) || radius <= 0.0 ||
        !Double::isFinite(origin.x()) || !Double::isFinite(origin.y()) || !Double::isFinite(origin.z()) ||
        !Double::isFinite(direction.x()) || !Double::isFinite(direction.y()) || !Double::isFinite(direction.z()) ||
        (direction.x() == 0 && direction.y() == 0 && direction.z() == 0)) {
        return RaySolidQueryStatus::InvalidInput;
    }
    const double lengthSquared = direction.dotProduct(direction);
    // C++ port note: the square is stored (volatile) before comparing it, as
    // Turbo C++ 3.0 and gcc 2.7.2 keep it in the x87 extended exponent range
    volatile double storedRadiusSquared = radius * radius;
    if (!Double::isFinite(lengthSquared) || lengthSquared == 0 ||
        !Double::isFinite(origin.dotProduct(origin)) || !Double::isFinite(storedRadiusSquared) ||
        storedRadiusSquared == 0.0) {
        return RaySolidQueryStatus::Indeterminate;
    }
    // Obtain the complete analytic interval before clipping: an empty set of
    // surface events in the requested range can still mean entirely inside.
    double nearT = 0, farT = 0;
    const int count = intersectionRoots(origin, direction, radius,
        RayCrossingQuery(-infinity, infinity), nearT, farT);
    if (!Double::isFinite(nearT) || !Double::isFinite(farT)) {
        return RaySolidQueryStatus::Indeterminate;
    }
    if (count < 2) {
        if (!result.insideSegments.add(FALSE)) {
            result.clear(); return RaySolidQueryStatus::Indeterminate;
        }
        return RaySolidQueryStatus::Supported; // Miss or point tangency (IN/OUT).
    }
    if (!result.insideSegments.add(query.minT > nearT && query.minT <= farT)) {
        result.clear(); return RaySolidQueryStatus::Indeterminate;
    }
    if (nearT >= query.minT && nearT <= query.maxT) {
        if (!result.boundaries.add(RaySolidBoundary(nearT, this)) || !result.insideSegments.add(TRUE)) {
            result.clear(); return RaySolidQueryStatus::Indeterminate;
        }
    }
    if (farT >= query.minT && farT <= query.maxT) {
        if (!result.boundaries.add(RaySolidBoundary(farT, this)) || !result.insideSegments.add(FALSE)) {
            result.clear(); return RaySolidQueryStatus::Indeterminate;
        }
    }
    return RaySolidQueryStatus::Supported;
}

void Sphere::getBounds(GeometryBounds& result) const
{
    if (!Double::isFinite(radius) || radius <= 0) {
        result.setIndeterminate();
        return;
    }
    result.setFinite(Vector3Dd(-radius,-radius,-radius), Vector3Dd(radius,radius,radius));
}

BOOLEAN Sphere::validateSurfaceQuery(const Ray& ray, const RayCrossingQuery& query,
    RayFirstHitStatus::Value& failure) const
{
    const Vector3Dd& p = ray.getOrigin();
    const Vector3Dd& d = ray.getDirection();
    if (Double::isNaN(query.minT) || Double::isNaN(query.maxT) ||
        query.minT > query.maxT || !Double::isFinite(query.minimumChordSquared) ||
        query.minimumChordSquared < 0 || Double::isNaN(query.minimumExteriorApproach) ||
        !Double::isFinite(radius) || radius <= 0 ||
        !Double::isFinite(p.x()) || !Double::isFinite(p.y()) || !Double::isFinite(p.z()) ||
        !Double::isFinite(d.x()) || !Double::isFinite(d.y()) || !Double::isFinite(d.z()) ||
        (d.x() == 0 && d.y() == 0 && d.z() == 0)) { failure = RayFirstHitStatus::InvalidInput; return FALSE; }
    const double lengthSquared = d.dotProduct(d);
    const double originSquared = p.dotProduct(p);
    const double rSquared = radius * radius;
    if (!Double::isFinite(lengthSquared) || lengthSquared == 0 ||
        !Double::isFinite(originSquared) || !Double::isFinite(rSquared) || rSquared == 0)
        { failure = RayFirstHitStatus::Indeterminate; return FALSE; }
    return TRUE;
}

RayFirstHitStatus::Value Sphere::queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
    RayHit* result, RayBoundary* boundary) const
{
    if (result) result->clear();
    if (boundary) *boundary = RayBoundary();
    const Vector3Dd& p = ray.getOrigin();
    const Vector3Dd& d = ray.getDirection();
    RayFirstHitStatus::Value failure;
    if (!validateSurfaceQuery(ray, query, failure)) return failure;
    double nearT, farT;
    if (intersectionRoots(p, d, radius, query, nearT, farT) == 0) return RayFirstHitStatus::Miss;
    if (!Double::isFinite(nearT)) return RayFirstHitStatus::Indeterminate;
    if (boundary) *boundary = RayBoundary(nearT, this);
    if (result) {
        result->setHitDistance(nearT);
        if (result->shouldStoreRay() || result->needsAnySurfaceData()) {
            const Ray hitRay = ray.withT(nearT);
            result->setRay(hitRay);
            if (result->needsAnySurfaceData()) {
                fillSurfaceDetail(hitRay, nearT, result);
                result->setRay(hitRay);
            }
        }
    }
    return RayFirstHitStatus::Hit;
}

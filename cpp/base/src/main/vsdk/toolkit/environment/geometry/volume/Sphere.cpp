#include "java/lang/Double.h"
#include "java/lang/IllegalStateException.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "vsdk/toolkit/common/statistics/RaytraceStatistics.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"

Sphere::Sphere(double r) : radius(r), radiusSquared(r * r) {
    addControlSpecification("double;radius;(0, INFINITE)",
        &Sphere::getRadius, &Sphere::setRadius);
}

Ray*
Sphere::doIntersectionFirstHit(const Ray& inoutRay) {
    double nearT, farT;
    if (intersectionRoots(inoutRay.getOrigin(), inoutRay.getDirection(), radius,
            RayCrossingQuery(0, java::Double::POSITIVE_INFINITY), nearT, farT) == 0) {
        return nullptr;
    }
    return new Ray(inoutRay.withT(nearT));
}

bool
Sphere::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) {
    return doIntersectionFirstHit(inRay,
        RayCrossingQuery(0, java::Double::POSITIVE_INFINITY), outHit);
}

bool
Sphere::doIntersectionFirstHit(const Ray& inRay, const RayCrossingQuery& query,
    RayHit* outHit) {
    double t, farT;
    if (intersectionRoots(inRay.getOrigin(), inRay.getDirection(), radius,
            query, t, farT) == 0) {
        return false;
    }

    if (outHit != nullptr) {
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
    return true;
}

void
Sphere::doExtraInformation(const Ray& inRay, double inT, RayHit* outData) {
    fillSurfaceDetail(inRay, inT, outData);
}

void
Sphere::fillSurfaceDetail(const Ray& inRay, double inT, RayHit* outData) const {
    if (outData == nullptr) {
        return;
    }

    outData->beginDetailEvaluation();
    RaytraceStatistics::recordGeometryDetailComputation();
    bool needsNormalVector = outData->needsNormal() || outData->needsTextureCoordinates() || outData->needsTangent();
    if (!outData->needsPoint() && !needsNormalVector) {
        return;
    }

    Vector3Dd point(
        inRay.getOrigin().x() + inT * inRay.getDirection().x(),
        inRay.getOrigin().y() + inT * inRay.getDirection().y(),
        inRay.getOrigin().z() + inT * inRay.getDirection().z());

    if (outData->needsPoint()) {
        outData->point = point;
        if (java::Double::isFinite(point.x()) && java::Double::isFinite(point.y()) && java::Double::isFinite(point.z()))
            outData->markDetailAvailable(RayHit::DETAIL_POINT);
    }

    Vector3Dd normal;
    if (needsNormalVector) {
        const double pointSquared = point.dotProduct(point);
        // Vector3Dd::normalized intentionally treats lengths below VSDK::EPSILON
        // as zero; a small but valid sphere still needs a unit surface normal.
        if (java::Double::isFinite(pointSquared) && pointSquared > 0 && pointSquared < VSDK::EPSILON)
            normal = point.multiply(1.0 / java::Math::sqrt(pointSquared));
        else
            normal = point.normalized();
        if (outData->needsNormal()) {
            outData->normal = normal;
            const double normalSquared = normal.dotProduct(normal);
            if (java::Double::isFinite(normalSquared) && normalSquared > 0 &&
                java::Double::isFinite(normal.x()) && java::Double::isFinite(normal.y()) && java::Double::isFinite(normal.z()))
                outData->markDetailAvailable(RayHit::DETAIL_NORMAL);
        }
    }

    if (!outData->needsTextureCoordinates() && !outData->needsTangent()) {
        return;
    }

    double theta;
    double phi = std::acos(normal.z());

    if (normal.x() > VSDK::EPSILON) {
        theta = std::atan(normal.y() / normal.x()) + 3 * M_PI / 2;
    }
    else if (normal.x() < VSDK::EPSILON) {
        theta = std::atan(normal.y() / normal.x()) + 3 * M_PI / 2;
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
        if (java::Double::isFinite(outData->u) && java::Double::isFinite(outData->v))
            outData->markDetailAvailable(RayHit::DETAIL_UV);
    }
    if (outData->needsTangent()) {
        outData->tangent = Vector3Dd(
            std::sin(theta - M_PI / 2),
            -std::cos(theta - M_PI / 2),
            0);
        if (java::Double::isFinite(outData->tangent.x()) && java::Double::isFinite(outData->tangent.y()))
            outData->markDetailAvailable(RayHit::DETAIL_TANGENT);
    }
}

GeometryContainment
Sphere::classifyBoxCertified(const Vector3Dd& center, const Vector3Dd& halfWidths) const {
    if (!java::Double::isFinite(radius) || radius <= 0) return GeometryContainment::InvalidInput;
    if (!FloatingInterval::isSupportedEnvironment()) return GeometryContainment::Unsupported;
    const double c[3] = {center.x(), center.y(), center.z()};
    const double h[3] = {halfWidths.x(), halfWidths.y(), halfWidths.z()};
    // Squared distances from the centre to the farthest and nearest box points.
    FloatingInterval farthest = FloatingInterval::exact(0), nearest = FloatingInterval::exact(0);
    for (int i = 0; i < 3; ++i) {
        const FloatingInterval range = FloatingInterval::around(c[i], h[i]);
        if (!range.isValid()) return GeometryContainment::InvalidInput;
        const double extreme = java::Math::max(java::Math::abs(range.getLower()), java::Math::abs(range.getUpper()));
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

GeometryContainment
Sphere::classifyPointCertified(const Vector3Dd& p, double distanceTolerance) const {
    if (!java::Double::isFinite(radius) || radius <= 0 ||
        !java::Double::isFinite(distanceTolerance) || distanceTolerance < 0 ||
        !java::Double::isFinite(p.x()) || !java::Double::isFinite(p.y()) ||
        !java::Double::isFinite(p.z())) return GeometryContainment::InvalidInput;
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

GeometryContainment
Sphere::classifyPoint(const Vector3Dd& p, double distanceTolerance) const {
    if (!java::Double::isFinite(radius) || radius <= 0 ||
        !java::Double::isFinite(distanceTolerance) || distanceTolerance < 0 ||
        !java::Double::isFinite(p.x()) || !java::Double::isFinite(p.y()) ||
        !java::Double::isFinite(p.z())) return GeometryContainment::InvalidInput;
    if (distanceTolerance>0) return classifyPointCertified(p, distanceTolerance);
    const double squared = p.dotProduct(p);
    const double radiusSquaredValue = radius * radius;
    if (!java::Double::isFinite(squared) || !java::Double::isFinite(radius + distanceTolerance) ||
        !java::Double::isFinite(radiusSquaredValue) || radiusSquaredValue == 0 ||
        (squared == 0 && (p.x() != 0 || p.y() != 0 || p.z() != 0)))
        return GeometryContainment::Indeterminate;
    const double distance = p.length();
    if (distance < radius - distanceTolerance) return GeometryContainment::Inside;
    if (distance > radius + distanceTolerance) return GeometryContainment::Outside;
    return GeometryContainment::Boundary;
}

int
Sphere::doContainmentTest(const Vector3Dd& p, double distanceTolerance) {
    const GeometryContainment result = classifyPoint(p, distanceTolerance);
    if (result == GeometryContainment::Inside) return INSIDE;
    if (result == GeometryContainment::Boundary) return LIMIT;
    if (result == GeometryContainment::Outside) return OUTSIDE;
    // The legacy integer contract has no representation for uncertainty.
    // Do not silently convert an indeterminate or failed query to outside.
    throw java::IllegalStateException("Sphere containment cannot be represented by the legacy integer result");
}

double*
Sphere::getMinMax() {
    double* minmax = new double[6];
    for (int i = 0; i < 3; i++) {
        minmax[i] = -radius;
    }
    for (int i = 3; i < 6; i++) {
        minmax[i] = radius;
    }
    return minmax;
}

double Sphere::getRadius() const { return radius; }
double Sphere::getRadiusSquared() const { return radiusSquared; }
void Sphere::setRadius(double value) { radius = value; radiusSquared = value * value; }

Vector3Dd Sphere::spherePosition(double theta, double t, double r) {
    double phi = (t - 0.5) * M_PI;
    return Vector3Dd(
        std::cos(phi) * std::cos(theta) * r,
        std::cos(phi) * std::sin(theta) * r,
        std::sin(phi) * r);
}

PolyhedralBoundedSolid* Sphere::exportToPolyhedralBoundedSolid() {
    return buildPolyhedralBoundedSolid(DEFAULT_MERIDIANS, DEFAULT_PARALLELS, false);
}

PolyhedralBoundedSolid* Sphere::exportToPolyhedralBoundedSolid(int meridians, int parallels,
    bool withQuadrilaterals) {
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
    bool withQuadrilaterals) {
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

Vector3Dd Sphere::spherePosition(double theta, double phi) {
    return Vector3Dd(
        std::cos(phi) * std::cos(theta) * radius,
        -std::cos(phi) * std::sin(theta) * radius,
        std::sin(phi) * radius);
}

Vector3Dd Sphere::sphereNormal(double theta, double phi) {
    return Vector3Dd(
        std::cos(phi) * std::cos(theta),
        -std::cos(phi) * std::sin(theta),
        std::sin(phi));
}

Vector3Dd Sphere::sphereTangent(double theta, double) {
    return Vector3Dd(
        std::sin(theta),
        std::cos(theta),
        0);
}

Vector3Dd Sphere::sphereBinormal(double theta, double phi) {
    return Vector3Dd(
        -std::sin(phi) * std::cos(theta),
        std::sin(phi) * std::sin(theta),
        std::cos(phi) * std::cos(theta) * std::cos(theta) +
        std::cos(phi) * std::sin(theta) * std::sin(theta));
}

RayCrossingSupport Sphere::emitRayCrossings(const Ray& ray,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    RayFirstHitStatus failure;
    if (!validateSurfaceQuery(ray, query, failure)) {
        return failure == RayFirstHitStatus::InvalidInput ? RayCrossingSupport::InvalidInput
            : RayCrossingSupport::Indeterminate;
    }
    double nearT, farT;
    const int count = intersectionRoots(ray.getOrigin(), ray.getDirection(),
        radius, query, nearT, farT);
    auto emit = [&](double t) {
        const Vector3Dd point = ray.getOrigin().add(ray.getDirection().multiply(t));
        const double orientation = point.dotProduct(ray.getDirection());
        const RayCrossing::Kind kind = orientation < 0 ? RayCrossing::Kind::Entering
            : orientation > 0 ? RayCrossing::Kind::Leaving : RayCrossing::Kind::Tangent;
        sink.crossing(RayCrossing(t, this, kind));
    };
    if (count > 0) emit(nearT);
    if (count > 1) emit(farT);
    return RayCrossingSupport::Supported;
}


RaySolidQueryStatus Sphere::classifyRay(const Ray& ray,
    const RayCrossingQuery& query, RaySolidClassification& result) const
{
    result.clear();
    const Vector3Dd& origin = ray.getOrigin();
    const Vector3Dd& direction = ray.getDirection();
    const double infinity = java::Double::POSITIVE_INFINITY;
    // Surface compatibility cutoffs may discard genuine boundaries and cannot
    // define a solid classification. Applications apply those to surface queries.
    if (java::Double::isNaN(query.minT) || java::Double::isNaN(query.maxT) || query.minT >= query.maxT ||
        query.minimumChordSquared != 0.0 || query.minimumExteriorApproach != -infinity ||
        !java::Double::isFinite(radius) || radius <= 0.0 ||
        !java::Double::isFinite(origin.x()) || !java::Double::isFinite(origin.y()) || !java::Double::isFinite(origin.z()) ||
        !java::Double::isFinite(direction.x()) || !java::Double::isFinite(direction.y()) || !java::Double::isFinite(direction.z()) ||
        (direction.x() == 0 && direction.y() == 0 && direction.z() == 0)) {
        return RaySolidQueryStatus::InvalidInput;
    }
    const double lengthSquared = direction.dotProduct(direction);
    if (!java::Double::isFinite(lengthSquared) || lengthSquared == 0 ||
        !java::Double::isFinite(origin.dotProduct(origin)) || !java::Double::isFinite(radius * radius) ||
        radius * radius == 0.0) {
        return RaySolidQueryStatus::Indeterminate;
    }
    // Obtain the complete analytic interval before clipping: an empty set of
    // surface events in the requested range can still mean entirely inside.
    double nearT = 0, farT = 0;
    const int count = intersectionRoots(origin, direction, radius,
        RayCrossingQuery(-infinity, infinity), nearT, farT);
    if (!java::Double::isFinite(nearT) || !java::Double::isFinite(farT)) {
        return RaySolidQueryStatus::Indeterminate;
    }
    if (count < 2) {
        if (!result.insideSegments.add(false)) {
            result.clear(); return RaySolidQueryStatus::Indeterminate;
        }
        return RaySolidQueryStatus::Supported; // Miss or point tangency (IN/OUT).
    }
    if (!result.insideSegments.add(query.minT > nearT && query.minT <= farT)) {
        result.clear(); return RaySolidQueryStatus::Indeterminate;
    }
    if (nearT >= query.minT && nearT <= query.maxT) {
        if (!result.boundaries.add(RaySolidBoundary(nearT, this)) || !result.insideSegments.add(true)) {
            result.clear(); return RaySolidQueryStatus::Indeterminate;
        }
    }
    if (farT >= query.minT && farT <= query.maxT) {
        if (!result.boundaries.add(RaySolidBoundary(farT, this)) || !result.insideSegments.add(false)) {
            result.clear(); return RaySolidQueryStatus::Indeterminate;
        }
    }
    return RaySolidQueryStatus::Supported;
}

void Sphere::getBounds(GeometryBounds& result) const
{
    if (!java::Double::isFinite(radius) || radius <= 0) {
        result.setIndeterminate();
        return;
    }
    result.setFinite(Vector3Dd(-radius,-radius,-radius), Vector3Dd(radius,radius,radius));
}

bool Sphere::validateSurfaceQuery(const Ray& ray, const RayCrossingQuery& query,
    RayFirstHitStatus& failure) const
{
    const Vector3Dd& p = ray.getOrigin();
    const Vector3Dd& d = ray.getDirection();
    if (java::Double::isNaN(query.minT) || java::Double::isNaN(query.maxT) ||
        query.minT > query.maxT || !java::Double::isFinite(query.minimumChordSquared) ||
        query.minimumChordSquared < 0 || java::Double::isNaN(query.minimumExteriorApproach) ||
        !java::Double::isFinite(radius) || radius <= 0 ||
        !java::Double::isFinite(p.x()) || !java::Double::isFinite(p.y()) || !java::Double::isFinite(p.z()) ||
        !java::Double::isFinite(d.x()) || !java::Double::isFinite(d.y()) || !java::Double::isFinite(d.z()) ||
        (d.x() == 0 && d.y() == 0 && d.z() == 0)) { failure = RayFirstHitStatus::InvalidInput; return false; }
    const double lengthSquared = d.dotProduct(d);
    const double originSquared = p.dotProduct(p);
    const double rSquared = radius * radius;
    if (!java::Double::isFinite(lengthSquared) || lengthSquared == 0 ||
        !java::Double::isFinite(originSquared) || !java::Double::isFinite(rSquared) || rSquared == 0)
        { failure = RayFirstHitStatus::Indeterminate; return false; }
    return true;
}

RayFirstHitStatus Sphere::queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
    RayHit* result, RayBoundary* boundary) const
{
    if (result) result->clear();
    if (boundary) *boundary = RayBoundary();
    const Vector3Dd& p = ray.getOrigin();
    const Vector3Dd& d = ray.getDirection();
    RayFirstHitStatus failure;
    if (!validateSurfaceQuery(ray, query, failure)) return failure;
    double nearT, farT;
    if (intersectionRoots(p, d, radius, query, nearT, farT) == 0) return RayFirstHitStatus::Miss;
    if (!java::Double::isFinite(nearT)) return RayFirstHitStatus::Indeterminate;
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

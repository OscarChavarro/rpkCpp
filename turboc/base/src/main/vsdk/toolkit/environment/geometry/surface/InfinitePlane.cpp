#include <stddef.h>
#include <math.h>

#include "java/lang/String.h"
#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/Cpp98Support.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/surface/QuadMesh.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/Cpp98Support.h"

/**
C++ port note: true when the binary64 product of two nonzero factors is zero.
The product is stored (volatile) first: Turbo C++ 3.0 and gcc 2.7.2 compare
it in the extended exponent range of the x87, where it does not underflow.
*/
static BOOLEAN
productUnderflows(double a, double b)
{
    volatile double product = a * b;
    return a != 0 && b != 0 && product == 0;
}
InfinitePlane::InfinitePlane(const InfinitePlane& other) { clone(other); }

InfinitePlane::InfinitePlane(double a_, double b_, double c_, double d_)
    : a(a_), b(b_), c(c_), d(d_) {}

InfinitePlane::InfinitePlane(const Vector3Dd& normal, const Vector3Dd& pointInPlane)
{
    Vector3Dd n = normal.normalized();
    a = n.x(); b = n.y(); c = n.z();
    d = -n.dotProduct(pointInPlane);
}

InfinitePlane::InfinitePlane(const Vector3Dd& p0, const Vector3Dd& p1, const Vector3Dd& p2)
{
    Vector3Dd aa = p1.subtract(p0).normalized();
    Vector3Dd bb = p2.subtract(p0).normalized();
    Vector3Dd normal = aa.crossProduct(bb).normalized();
    a = normal.x(); b = normal.y(); c = normal.z();
    d = -normal.dotProduct(p0);
}

void InfinitePlane::clone(const InfinitePlane& other)
{
    a = other.a; b = other.b; c = other.c; d = other.d;
}

void InfinitePlane::getBounds(GeometryBounds& result) const
{
    if (!Double::isFinite(a) || !Double::isFinite(b) ||
        !Double::isFinite(c) || !Double::isFinite(d) ||
        (a == 0 && b == 0 && c == 0)) result.setIndeterminate();
    else result.setUnbounded();
}

RayFirstHitStatus::Value InfinitePlane::evaluateRay(const Ray& ray, const RayCrossingQuery& query,
    double& depth) const
{
    const Vector3Dd& origin = ray.getOrigin();
    const Vector3Dd& direction = ray.getDirection();
    if (!Double::isFinite(a) || !Double::isFinite(b) ||
        !Double::isFinite(c) || !Double::isFinite(d) ||
        (a == 0 && b == 0 && c == 0) ||
        !Double::isFinite(origin.x()) || !Double::isFinite(origin.y()) ||
        !Double::isFinite(origin.z()) || !Double::isFinite(direction.x()) ||
        !Double::isFinite(direction.y()) || !Double::isFinite(direction.z()) ||
        (direction.x() == 0 && direction.y() == 0 && direction.z() == 0) ||
        Double::isNaN(query.minT) || Double::isNaN(query.maxT) || query.minT > query.maxT ||
        !Double::isFinite(query.minimumChordSquared) || query.minimumChordSquared < 0 ||
        Double::isNaN(query.minimumExteriorApproach)) return RayFirstHitStatus::InvalidInput;
    if (query.minimumChordSquared != 0 || query.minimumExteriorApproach != Double::NEGATIVE_INFINITY)
        return RayFirstHitStatus::Unsupported;
    const Vector3Dd normal(a,b,c);
    const double squared = normal.dotProduct(normal);
    const double numerator = -(normal.dotProduct(origin) + d);
    const double denominator = normal.dotProduct(direction);
    if (!Double::isFinite(squared) || squared == 0 ||
        !Double::isFinite(numerator) || !Double::isFinite(denominator))
        return RayFirstHitStatus::Indeterminate;
    if (denominator == 0 &&
        ((productUnderflows(a, direction.x())) ||
         (productUnderflows(b, direction.y())) ||
         (productUnderflows(c, direction.z()))))
        return RayFirstHitStatus::Indeterminate;
    const RayFirstHitStatus::Value status = intersectionDepth(numerator, denominator, 0,
        query.minT, query.maxT, TRUE, depth);
    if (status != RayFirstHitStatus::Hit) return status;
    const Vector3Dd point = origin.add(direction.multiply(depth));
    if (!Double::isFinite(point.x()) || !Double::isFinite(point.y()) ||
        !Double::isFinite(point.z())) return RayFirstHitStatus::Indeterminate;
    return RayFirstHitStatus::Hit;
}

RayCrossingSupport::Value InfinitePlane::emitRayCrossings(const Ray& ray,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    double depth;
    const RayFirstHitStatus::Value status = evaluateRay(ray, query, depth);
    if (status == RayFirstHitStatus::Unsupported) return RayCrossingSupport::Unsupported;
    if (status == RayFirstHitStatus::InvalidInput) return RayCrossingSupport::InvalidInput;
    if (status == RayFirstHitStatus::Indeterminate) return RayCrossingSupport::Indeterminate;
    if (status == RayFirstHitStatus::Hit)
        sink.crossing(RayCrossing(depth, this, RayCrossingKind::Unclassified));
    return RayCrossingSupport::Supported;
}

RayFirstHitStatus::Value InfinitePlane::queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
    RayHit* result, RayBoundary* boundary) const
{
    if (result) result->clear();
    if (boundary) *boundary = RayBoundary();
    double depth;
    const RayFirstHitStatus::Value status = evaluateRay(ray, query, depth);
    if (status != RayFirstHitStatus::Hit) return status;
    if (boundary) *boundary = RayBoundary(depth, this);
    if (result) {
        result->setHitDistance(depth);
        if (result->shouldStoreRay()) result->setRay(ray.withT(depth));
        if (result->needsAnySurfaceData()) ((InfinitePlane*)(this))->doExtraInformation(ray, depth, result);
    }
    return RayFirstHitStatus::Hit;
}

RayFirstHitStatus::Value InfinitePlane::querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
    RayHit* hit) const
{
    if (hit) hit->clear();
    if (!hit || boundary.geometry != this || !Double::isFinite(boundary.t))
        return RayFirstHitStatus::InvalidInput;
    if (boundary.getInstanceCount() != 0) return RayFirstHitStatus::Unsupported;
    const RayFirstHitStatus::Value status = queryFirstHit(ray, RayCrossingQuery(boundary.t, boundary.t), NULL);
    if (status != RayFirstHitStatus::Hit) return status;
    hit->setHitDistance(boundary.t);
    if (hit->shouldStoreRay()) hit->setRay(ray.withT(boundary.t));
    ((InfinitePlane*)(this))->doExtraInformation(ray, boundary.t, hit);
    if (boundary.normalReversed && hit->needsNormal() &&
        (hit->getAvailableDetailMask() & RayHit::DETAIL_NORMAL))
        hit->normal = hit->normal.multiply(-1);
    return RayFirstHitStatus::Hit;
}

Vector3Dd InfinitePlane::surfaceNormal() const
{
    const Vector3Dd normal(a,b,c);
    const double squared = normal.dotProduct(normal);
    if (squared > 0 && squared < VSDK::EPSILON) return normal.multiply(1.0 / Math::sqrt(squared));
    return normal.normalized();
}

Ray* InfinitePlane::doIntersectionFirstHit(const Ray& inout_rayo)
{
    double denominator = a*inout_rayo.getDirection().x() + b*inout_rayo.getDirection().y() + c*inout_rayo.getDirection().z();
    if (Cpp98Support::abs(denominator) < VSDK::EPSILON) return NULL;
    double t = -(a*inout_rayo.getOrigin().x() + b*inout_rayo.getOrigin().y() + c*inout_rayo.getOrigin().z() + d)/denominator;
    if (t < 0) return NULL;
    return new Ray(inout_rayo.withT(t));
}

BOOLEAN InfinitePlane::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit)
{
    Ray* hit = doIntersectionFirstHit(inRay);
    if (hit == NULL) return FALSE;
    if (outHit != NULL) {
        outHit->setRay(*hit);
        doExtraInformation(*hit, hit->getT(), outHit);
    }
    delete hit;
    return TRUE;
}

Ray* InfinitePlane::doIntersectionWithNegative(const Ray& inout_rayo)
{
    double denominator = a*inout_rayo.getDirection().x() + b*inout_rayo.getDirection().y() + c*inout_rayo.getDirection().z();
    if (Cpp98Support::abs(denominator) < VSDK::EPSILON) {
        Ray r(inout_rayo.getOrigin(), inout_rayo.getDirection().multiply(-1));
        Ray* hit = doIntersectionFirstHit(r);
        if (hit != NULL) {
            Ray* out = new Ray(inout_rayo.withT(-hit->getT()));
            delete hit;
            return out;
        }
        return NULL;
    }
    double t = -(a*inout_rayo.getOrigin().x() + b*inout_rayo.getOrigin().y() + c*inout_rayo.getOrigin().z() + d)/denominator;
    return new Ray(inout_rayo.withT(t));
}

int InfinitePlane::doContainmentTestHalfSpace(const Vector3Dd& p, double distanceTolerance)
{
    double num = a*p.x() + b*p.y() + c*p.z() + d;
    int op = LIMIT;
    if (num > distanceTolerance) op = OUTSIDE;
    else if (num < -distanceTolerance) op = INSIDE;
    return op;
}

int InfinitePlane::doContainmentTest(const Vector3Dd& p, double distanceTolerance)
{
    double num = a*p.x() + b*p.y() + c*p.z() + d;
    int op = LIMIT;
    if (num > distanceTolerance) op = OUTSIDE;
    else if (num < -distanceTolerance) op = -INSIDE;
    return op;
}

void InfinitePlane::doExtraInformation(const Ray& inRay, double inT, RayHit* outData)
{
    if (outData == NULL) return;
    outData->beginDetailEvaluation();
    outData->point = Vector3Dd(
        inRay.getOrigin().x() + inT*inRay.getDirection().x(),
        inRay.getOrigin().y() + inT*inRay.getDirection().y(),
        inRay.getOrigin().z() + inT*inRay.getDirection().z());
    outData->normal = surfaceNormal();
    if (outData->needsPoint()) outData->markDetailAvailable(RayHit::DETAIL_POINT);
    if (outData->needsNormal()) outData->markDetailAvailable(RayHit::DETAIL_NORMAL);
}

double* InfinitePlane::getMinMax()
{
    double* minmax = new double[6];
    int i;
    for (i = 0; i < 3; i++) minmax[i] = -1e308;
    for (i = 3; i < 6; i++) minmax[i] = 1e308;
    return minmax;
}

Vector3Dd InfinitePlane::getNormal() const { return Vector3Dd(a, b, c).normalized(); }
double InfinitePlane::getD() const { return d; }

void InfinitePlane::setNormal(const Vector3Dd& n)
{
    Vector3Dd nn = n.normalized();
    a = nn.x(); b = nn.y(); c = nn.z();
}

void InfinitePlane::setD(double d_) { d = d_; }

void InfinitePlane::setFromPointNormal(const Vector3Dd& p, const Vector3Dd& n)
{
    setNormal(n);
    d = -(n.x()*p.x() + n.y()*p.y() + n.z()*p.z());
}

double InfinitePlane::pointDistance(const Vector3Dd& p) const { return a*p.x()+b*p.y()+c*p.z()+d; }

Vector3Dd InfinitePlane::projectPoint(const Vector3Dd& p) const
{
    double distance = pointDistance(p);
    Vector3Dd n(a, b, c);
    n = n.normalized().multiply(distance);
    return p.subtract(n);
}

Vector3Dd InfinitePlane::mirrorPoint(const Vector3Dd& p) const
{
    double distance = pointDistance(p);
    Vector3Dd n(a, b, c);
    n = n.normalized().multiply(2*distance);
    return p.subtract(n);
}

BOOLEAN InfinitePlane::overlapsWith(const InfinitePlane& other, double tolerance) const
{
    double a1=a,b1=b,c1=c,d1=d;
    double a2=other.a,b2=other.b,c2=other.c,d2=other.d;
    Vector3Dd n1(a1,b1,c1), n2(a2,b2,c2);
    double l1=n1.length(), l2=n2.length();
    a1/=l1; b1/=l1; c1/=l1; d1/=l1;
    a2/=l2; b2/=l2; c2/=l2; d2/=l2;
    return Cpp98Support::abs(a2-a1)<=tolerance && Cpp98Support::abs(b2-b1)<=tolerance && Cpp98Support::abs(c2-c1)<=tolerance && Cpp98Support::abs(d2-d1)<=tolerance;
}

String InfinitePlane::toString() const
{
    return "InfinitePlane: N=<" + VSDK::formatDouble(a) + ", " + VSDK::formatDouble(b) + ", " +
        VSDK::formatDouble(c) + ">, D=" + VSDK::formatDouble(d);
}

double InfinitePlane::getA() const { return a; }
void InfinitePlane::setA(double a_) { a = a_; }
double InfinitePlane::getB() const { return b; }
void InfinitePlane::setB(double b_) { b = b_; }
double InfinitePlane::getC() const { return c; }
void InfinitePlane::setC(double c_) { c = c_; }

QuadMesh* InfinitePlane::exportToQuadMesh(const Vector3Dd& nearPoint,
    double size, int tiles) const
{
    double length = Math::sqrt(a*a + b*b + c*c);
    if ( !(length > 0) || tiles < 1 ) {
        return NULL;
    }
    Vector3Dd n = getNormal();
    Vector3Dd origin = n.multiply(-d / length);
    Vector3Dd reference = Math::abs(n.z()) < 0.9 ?
        Vector3Dd(0, 0, 1) : Vector3Dd(1, 0, 0);
    Vector3Dd u = reference.crossProduct(n).normalized();
    Vector3Dd v = n.crossProduct(u).normalized();
    double cell = size / tiles;
    Vector3Dd offset = projectPoint(nearPoint).subtract(origin);
    double cu = Math::rint(offset.dotProduct(u) / cell) * cell;
    double cv = Math::rint(offset.dotProduct(v) / cell) * cell;
    Vector3Dd corner = origin.add(u.multiply(cu - size/2))
        .add(v.multiply(cv - size/2));
    int vertices = tiles + 1;
    QuadMesh* mesh = new QuadMesh();
    int i;
    int j;
    long index;

    mesh->initVertexPositionsArray(vertices*vertices);
    ArrayList<double>& positions = mesh->getVertexPositions();
    for ( i = 0; i < vertices; i++ ) {
        for ( j = 0; j < vertices; j++ ) {
            Vector3Dd p = corner.add(u.multiply(i * cell))
                .add(v.multiply(j * cell));
            index = ((long)i)*vertices + j;
            positions[3*index] = p.x();
            positions[3*index+1] = p.y();
            positions[3*index+2] = p.z();
        }
    }
    mesh->initQuadArrays(tiles*tiles);
    index = 0;
    for ( i = 0; i < tiles; i++ ) {
        for ( j = 0; j < tiles; j++ ) {
            // Counterclockwise seen from the normal
            mesh->setQuadAt((int)index, i*vertices + j, (i+1)*vertices + j,
                (i+1)*vertices + j+1, i*vertices + j+1);
            index++;
        }
    }
    return mesh;
}

const Class InfinitePlane::CLASS = { "InfinitePlane", &HalfSpace::CLASS };

const Class *
InfinitePlane::getClass() const
{
    return &CLASS;
}

//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#include <cmath>

#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/volume/Tube.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerConstructions.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

Tube::Tube(double internalRadius, double externalRadius, double height) :
    internalRadius(internalRadius), externalRadius(externalRadius),
    height(height)
{
    addControlSpecification("double;internalRadius;[0, INFINITE)",
        &Tube::getInternalRadius, &Tube::setInternalRadius);
    addControlSpecification("double;externalRadius;(0, INFINITE)",
        &Tube::getExternalRadius, &Tube::setExternalRadius);
    addControlSpecification("double;height;[0, INFINITE)",
        &Tube::getHeight, &Tube::setHeight);
}

double
Tube::getInternalRadius() const
{
    return internalRadius;
}

double
Tube::getExternalRadius() const
{
    return externalRadius;
}

double
Tube::getHeight() const
{
    return height;
}

void
Tube::setInternalRadius(double value)
{
    internalRadius = value;
}

void
Tube::setExternalRadius(double value)
{
    externalRadius = value;
}

void
Tube::setHeight(double value)
{
    height = value;
}

bool
Tube::isWasher() const
{
    return height <= 0.0;
}

Geometry*
Tube::copyGeometry() const
{
    return new Tube(internalRadius, externalRadius, height);
}

bool
Tube::hasValidParameters() const
{
    return java::Double::isFinite(internalRadius) &&
        java::Double::isFinite(externalRadius) &&
        java::Double::isFinite(height) && internalRadius >= 0 &&
        externalRadius > internalRadius && height >= 0;
}

//= Interior of the tube along a ray =====================================

bool
Tube::cylinderInterval(const Vector3Dd& origin, const Vector3Dd& direction,
    double radius, int surface, Interval& out)
{
    double ox = origin.x();
    double oy = origin.y();
    double dx = direction.x();
    double dy = direction.y();
    double a = dx * dx + dy * dy;
    double c = ox * ox + oy * oy - radius * radius;

    if ( a <= 0.0 ) {
        // Parallel to the axis: everything or nothing
        if ( c < 0.0 ) {
            out = Interval(java::Double::NEGATIVE_INFINITY, SURFACE_NONE,
                java::Double::POSITIVE_INFINITY, SURFACE_NONE);
            return true;
        }
        return false;
    }
    double b = 2.0 * (ox * dx + oy * dy);
    double discriminant = b * b - 4.0 * a * c;

    if ( discriminant <= 0.0 ) {
        return false;
    }
    double root = java::Math::sqrt(discriminant);
    // Numerically stable roots
    double q = (b < 0.0) ? -0.5 * (b - root) : -0.5 * (b + root);
    double t0 = q / a;
    double t1 = (q != 0.0) ? c / q : -t0;

    if ( t0 > t1 ) {
        double swap = t0;
        t0 = t1;
        t1 = swap;
    }
    out = Interval(t0, surface, t1, surface);
    return true;
}

bool
Tube::slabInterval(const Vector3Dd& origin, const Vector3Dd& direction,
    Interval& out) const
{
    double oz = origin.z();
    double dz = direction.z();

    if ( dz == 0.0 ) {
        if ( oz > 0.0 && oz < height ) {
            out = Interval(java::Double::NEGATIVE_INFINITY, SURFACE_NONE,
                java::Double::POSITIVE_INFINITY, SURFACE_NONE);
            return true;
        }
        return false;
    }
    double tBottom = -oz / dz;
    double tTop = (height - oz) / dz;

    if ( tBottom < tTop ) {
        out = Interval(tBottom, SURFACE_BOTTOM, tTop, SURFACE_TOP);
        return true;
    }
    out = Interval(tTop, SURFACE_TOP, tBottom, SURFACE_BOTTOM);
    return true;
}

int
Tube::computeInteriorIntervals(const Vector3Dd& origin,
    const Vector3Dd& direction, Interval out[2]) const
{
    if ( isWasher() ) {
        return 0;
    }
    Interval slab;
    if ( !slabInterval(origin, direction, slab) ) {
        return 0;
    }
    Interval external;
    if ( !cylinderInterval(origin, direction, externalRadius,
             SURFACE_EXTERNAL, external) ) {
        return 0;
    }

    //- Slab intersected with the external cylinder --------------------
    Interval body(slab.start, slab.startSurface, slab.end, slab.endSurface);
    if ( external.start > body.start ) {
        body.start = external.start;
        body.startSurface = external.startSurface;
    }
    if ( external.end < body.end ) {
        body.end = external.end;
        body.endSurface = external.endSurface;
    }
    if ( !(body.start < body.end) ) {
        return 0;
    }

    //- Minus the internal cylinder ------------------------------------
    Interval hole;
    bool withHole = false;
    if ( internalRadius > 0.0 ) {
        withHole = cylinderInterval(origin, direction, internalRadius,
            SURFACE_INTERNAL, hole);
    }
    if ( !withHole || hole.end <= body.start || hole.start >= body.end ) {
        out[0] = body;
        return 1;
    }

    int n = 0;
    if ( hole.start > body.start ) {
        out[n++] = Interval(body.start, body.startSurface, hole.start,
            SURFACE_INTERNAL);
    }
    if ( hole.end < body.end ) {
        out[n++] = Interval(hole.end, SURFACE_INTERNAL, body.end,
            body.endSurface);
    }
    return n;
}

double
Tube::washerHit(const Vector3Dd& origin, const Vector3Dd& direction) const
{
    double dz = direction.z();

    if ( dz == 0.0 ) {
        return java::Double::NaN;
    }
    double t = -origin.z() / dz;
    double px = origin.x() + direction.x() * t;
    double py = origin.y() + direction.y() * t;
    double r2 = px * px + py * py;

    if ( r2 > externalRadius * externalRadius ||
         r2 < internalRadius * internalRadius ) {
        return java::Double::NaN;
    }
    return t;
}

double
Tube::firstSurfaceHit(const Ray& ray, double minT, int& surface) const
{
    Vector3Dd origin = ray.getOrigin();
    Vector3Dd direction = ray.getDirection();

    if ( isWasher() ) {
        double t = washerHit(origin, direction);
        if ( !java::Double::isNaN(t) && t > minT ) {
            surface = direction.z() < 0 ? SURFACE_TOP : SURFACE_BOTTOM;
            return t;
        }
        return java::Double::NaN;
    }

    Interval intervals[2];
    int n = computeInteriorIntervals(origin, direction, intervals);
    for ( int i = 0; i < n; i++ ) {
        if ( intervals[i].start > minT ) {
            surface = intervals[i].startSurface;
            return intervals[i].start;
        }
        if ( intervals[i].end > minT ) {
            surface = intervals[i].endSurface;
            return intervals[i].end;
        }
    }
    return java::Double::NaN;
}

//= Classic ray intersection interface ===================================

Ray*
Tube::doIntersectionFirstHit(const Ray& inOutRay)
{
    RayHit hit;
    if ( doIntersectionFirstHit(inOutRay, &hit) && hit.getRay() != nullptr ) {
        return new Ray(*hit.getRay());
    }
    return nullptr;
}

bool
Tube::doIntersectionFirstHit(const Ray& inOutRay, RayHit* outHit)
{
    if ( !hasValidParameters() ) {
        return false;
    }
    int surface = SURFACE_NONE;
    double t = firstSurfaceHit(inOutRay, VSDK::EPSILON, surface);

    if ( java::Double::isNaN(t) ) {
        return false;
    }
    if ( outHit != nullptr ) {
        if ( outHit->shouldStoreRay() || outHit->needsAnySurfaceData() ) {
            outHit->setRay(inOutRay.withT(t));
        }
        else {
            outHit->setHitDistance(t);
        }
        if ( outHit->needsAnySurfaceData() ) {
            fillSurfaceData(inOutRay, t, surface, outHit);
        }
    }
    return true;
}

void
Tube::doExtraInformation(const Ray& inRay, double inT, RayHit* outData)
{
    if ( outData == nullptr ) {
        return;
    }
    Vector3Dd p = inRay.getOrigin().add(inRay.getDirection().multiply(inT));
    fillSurfaceData(inRay, inT, surfaceAt(p, inRay.getDirection()), outData);
}

int
Tube::surfaceAt(const Vector3Dd& p, const Vector3Dd& rayDirection) const
{
    if ( isWasher() ) {
        return rayDirection.z() < 0 ? SURFACE_TOP : SURFACE_BOTTOM;
    }
    double rho = java::Math::sqrt(p.x() * p.x() + p.y() * p.y());
    double dBottom = java::Math::abs(p.z());
    double dTop = java::Math::abs(p.z() - height);
    double dExternal = java::Math::abs(rho - externalRadius);
    double dInternal = internalRadius > 0 ?
        java::Math::abs(rho - internalRadius) : java::Double::POSITIVE_INFINITY;
    int surface = SURFACE_BOTTOM;
    double best = dBottom;

    if ( dTop < best ) {
        best = dTop;
        surface = SURFACE_TOP;
    }
    if ( dExternal < best ) {
        best = dExternal;
        surface = SURFACE_EXTERNAL;
    }
    if ( dInternal < best ) {
        surface = SURFACE_INTERNAL;
    }
    return surface;
}

void
Tube::fillSurfaceData(const Ray& ray, double t, int surface,
    RayHit* outData) const
{
    outData->beginDetailEvaluation();
    Vector3Dd p = ray.getOrigin().add(ray.getDirection().multiply(t));
    double rho = java::Math::sqrt(p.x() * p.x() + p.y() * p.y());
    double angle = std::atan2(p.y(), p.x());
    if ( angle < 0 ) {
        angle += 2 * java::Math::PI;
    }
    double c = rho > 0 ? p.x() / rho : 1.0;
    double s = rho > 0 ? p.y() / rho : 0.0;
    Vector3Dd normal;

    switch ( surface ) {
      case SURFACE_TOP:
        normal = Vector3Dd(0, 0, 1);
        break;
      case SURFACE_BOTTOM:
        normal = Vector3Dd(0, 0, -1);
        break;
      case SURFACE_INTERNAL:
        normal = Vector3Dd(-c, -s, 0);
        break;
      default:
        normal = Vector3Dd(c, s, 0);
        break;
    }

    if ( outData->needsPoint() ) {
        outData->point = p;
        outData->markDetailAvailable(RayHit::DETAIL_POINT);
    }
    if ( outData->needsNormal() ) {
        outData->normal = normal;
        outData->markDetailAvailable(RayHit::DETAIL_NORMAL);
    }
    if ( outData->needsTangent() ) {
        outData->tangent = Vector3Dd(-s, c, 0);
        outData->markDetailAvailable(RayHit::DETAIL_TANGENT);
    }
    if ( outData->needsTextureCoordinates() ) {
        outData->u = angle / (2 * java::Math::PI);
        if ( surface == SURFACE_TOP || surface == SURFACE_BOTTOM ) {
            outData->v = (rho - internalRadius) /
                (externalRadius - internalRadius);
        }
        else {
            outData->v = height > 0 ? p.z() / height : 0;
        }
        outData->markDetailAvailable(RayHit::DETAIL_UV);
    }
}

//= Ray crossing and classification interface ============================

bool
Tube::validateQuery(const Ray& ray, const RayCrossingQuery& query,
    RayFirstHitStatus& failure) const
{
    if ( java::Double::isNaN(query.minT) || java::Double::isNaN(query.maxT) ||
         query.minT > query.maxT ||
         !java::Double::isFinite(query.minimumChordSquared) ||
         query.minimumChordSquared < 0 ||
         java::Double::isNaN(query.minimumExteriorApproach) ) {
        failure = RayFirstHitStatus::InvalidInput;
        return true;
    }
    if ( query.minimumChordSquared != 0 ||
         query.minimumExteriorApproach != java::Double::NEGATIVE_INFINITY ) {
        failure = RayFirstHitStatus::Unsupported;
        return true;
    }
    Vector3Dd o = ray.getOrigin();
    Vector3Dd d = ray.getDirection();
    if ( !java::Double::isFinite(o.x()) || !java::Double::isFinite(o.y()) ||
         !java::Double::isFinite(o.z()) || !java::Double::isFinite(d.x()) ||
         !java::Double::isFinite(d.y()) || !java::Double::isFinite(d.z()) ||
         (d.x() == 0 && d.y() == 0 && d.z() == 0) ) {
        failure = RayFirstHitStatus::InvalidInput;
        return true;
    }
    if ( !hasValidParameters() ) {
        failure = RayFirstHitStatus::InvalidInput;
        return true;
    }
    return false;
}

RayCrossingSupport
Tube::emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
    RayCrossingSink& sink) const
{
    RayFirstHitStatus invalid;
    if ( validateQuery(ray, query, invalid) ) {
        if ( invalid == RayFirstHitStatus::Unsupported ) {
            return RayCrossingSupport::Unsupported;
        }
        return RayCrossingSupport::InvalidInput;
    }

    if ( isWasher() ) {
        double t = washerHit(ray.getOrigin(), ray.getDirection());
        if ( !java::Double::isNaN(t) && t >= query.minT && t <= query.maxT ) {
            sink.crossing(RayCrossing(t, this, RayCrossingKind::Tangent));
        }
        return RayCrossingSupport::Supported;
    }

    Interval intervals[2];
    int n = computeInteriorIntervals(ray.getOrigin(), ray.getDirection(),
        intervals);
    for ( int i = 0; i < n; i++ ) {
        if ( !java::Double::isFinite(intervals[i].start) ||
             !java::Double::isFinite(intervals[i].end) ) {
            return RayCrossingSupport::Indeterminate;
        }
    }
    for ( int i = 0; i < n; i++ ) {
        double entry = intervals[i].start;
        double exit = intervals[i].end;
        if ( entry >= query.minT && entry <= query.maxT ) {
            sink.crossing(RayCrossing(entry, this, RayCrossingKind::Entering));
        }
        if ( exit >= query.minT && exit <= query.maxT ) {
            sink.crossing(RayCrossing(exit, this, RayCrossingKind::Leaving));
        }
    }
    return RayCrossingSupport::Supported;
}

RayFirstHitStatus
Tube::queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
    RayHit* result, RayBoundary* boundary) const
{
    if ( result != nullptr ) {
        result->clear();
    }
    if ( boundary != nullptr ) {
        *boundary = RayBoundary();
    }
    RayFirstHitStatus invalid;
    if ( validateQuery(ray, query, invalid) ) {
        return invalid;
    }

    // The first boundary at or after minT: the surface hit is searched
    // strictly after a parameter slightly smaller than minT
    int surface = SURFACE_NONE;
    double t = firstSurfaceHit(ray,
        java::Math::nextAfter(query.minT, java::Double::NEGATIVE_INFINITY),
        surface);

    if ( java::Double::isNaN(t) || t > query.maxT ) {
        return RayFirstHitStatus::Miss;
    }
    if ( boundary != nullptr ) {
        *boundary = RayBoundary(t, this);
    }
    if ( result != nullptr ) {
        result->setHitDistance(t);
        if ( result->shouldStoreRay() ) {
            result->setRay(ray.withT(t));
        }
        if ( result->needsAnySurfaceData() ) {
            fillSurfaceData(ray, t, surface, result);
        }
    }
    return RayFirstHitStatus::Hit;
}

RayFirstHitStatus
Tube::querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
    RayHit* hit) const
{
    if ( hit != nullptr ) {
        hit->clear();
    }
    if ( hit == nullptr || boundary.geometry != this ||
         !java::Double::isFinite(boundary.t) ) {
        return RayFirstHitStatus::InvalidInput;
    }
    if ( boundary.getInstanceCount() != 0 ) {
        return RayFirstHitStatus::Unsupported;
    }
    RayFirstHitStatus status = queryFirstHit(ray,
        RayCrossingQuery(boundary.t, boundary.t), nullptr);
    if ( status != RayFirstHitStatus::Hit ) {
        return status;
    }
    hit->setHitDistance(boundary.t);
    if ( hit->shouldStoreRay() ) {
        hit->setRay(ray.withT(boundary.t));
    }
    const_cast<Tube*>(this)->doExtraInformation(ray, boundary.t, hit);
    if ( boundary.normalReversed && hit->needsNormal() &&
         (hit->getAvailableDetailMask() & RayHit::DETAIL_NORMAL) != 0 ) {
        hit->normal = hit->normal.multiply(-1);
    }
    return RayFirstHitStatus::Hit;
}

RaySolidQueryStatus
Tube::classifyRay(const Ray& ray, const RayCrossingQuery& query,
    RaySolidClassification& result) const
{
    result.clear();
    if ( query.minT >= query.maxT ) {
        return RaySolidQueryStatus::InvalidInput;
    }
    RayFirstHitStatus invalid;
    if ( validateQuery(ray, query, invalid) ) {
        if ( invalid == RayFirstHitStatus::Unsupported ) {
            return RaySolidQueryStatus::Unsupported;
        }
        return RaySolidQueryStatus::InvalidInput;
    }

    Interval intervals[2];
    int n = computeInteriorIntervals(ray.getOrigin(), ray.getDirection(),
        intervals);
    bool startsInside = false;

    for ( int i = 0; i < n; i++ ) {
        if ( query.minT > intervals[i].start &&
             query.minT <= intervals[i].end ) {
            startsInside = true;
        }
    }
    result.insideSegments.add(startsInside);
    for ( int i = 0; i < n; i++ ) {
        double entry = intervals[i].start;
        double exit = intervals[i].end;
        if ( entry >= query.minT && entry <= query.maxT ) {
            result.boundaries.add(RaySolidBoundary(entry, this));
            result.insideSegments.add(true);
        }
        if ( exit >= query.minT && exit <= query.maxT ) {
            result.boundaries.add(RaySolidBoundary(exit, this));
            result.insideSegments.add(false);
        }
    }
    return RaySolidQueryStatus::Supported;
}

//= Containment ==========================================================

int
Tube::doContainmentTest(const Vector3Dd& p, double distanceTolerance)
{
    if ( !hasValidParameters() ) {
        return OUTSIDE;
    }
    double z = p.z();
    double rho = java::Math::sqrt(p.x() * p.x() + p.y() * p.y());

    if ( z < -distanceTolerance || z > height + distanceTolerance ||
         rho > externalRadius + distanceTolerance ||
         rho < internalRadius - distanceTolerance ) {
        return OUTSIDE;
    }
    bool insideHole = internalRadius > 0 && rho < internalRadius;
    if ( insideHole && rho < internalRadius - distanceTolerance ) {
        return OUTSIDE;
    }
    if ( z > distanceTolerance && z < height - distanceTolerance &&
         rho < externalRadius - distanceTolerance &&
         (internalRadius <= 0 || rho > internalRadius + distanceTolerance) ) {
        return INSIDE;
    }
    return LIMIT;
}

GeometryContainment
Tube::classifyPoint(const Vector3Dd& p, double distanceTolerance) const
{
    if ( !java::Double::isFinite(distanceTolerance) || distanceTolerance < 0 ||
         !java::Double::isFinite(p.x()) || !java::Double::isFinite(p.y()) ||
         !java::Double::isFinite(p.z()) ) {
        return GeometryContainment::InvalidInput;
    }
    if ( !hasValidParameters() ) {
        return GeometryContainment::InvalidInput;
    }
    int c = const_cast<Tube*>(this)->doContainmentTest(p, distanceTolerance);
    if ( c == INSIDE ) {
        return GeometryContainment::Inside;
    }
    if ( c == LIMIT ) {
        return GeometryContainment::Boundary;
    }
    return GeometryContainment::Outside;
}

//= Bounds ===============================================================

void
Tube::getBounds(GeometryBounds& result) const
{
    if ( !hasValidParameters() ) {
        result.setIndeterminate();
        return;
    }
    result.setFinite(Vector3Dd(-externalRadius, -externalRadius, 0),
        Vector3Dd(externalRadius, externalRadius, height));
}

double*
Tube::getMinMax()
{
    double* minmax = new double[6];

    minmax[0] = -externalRadius;
    minmax[1] = -externalRadius;
    minmax[2] = 0;
    minmax[3] = externalRadius;
    minmax[4] = externalRadius;
    minmax[5] = height;
    return minmax;
}

//= Boundary representation ==============================================

PolyhedralBoundedSolid*
Tube::exportToPolyhedralBoundedSolid()
{
    return buildPolyhedralBoundedSolid(DEFAULT_CIRCUMFERENCE_DIVISIONS);
}

PolyhedralBoundedSolid*
Tube::exportToPolyhedralBoundedSolid(int circumferenceDivisions)
{
    return buildPolyhedralBoundedSolid(
        java::Math::max(MIN_CIRCUMFERENCE_DIVISIONS, circumferenceDivisions));
}

PolyhedralBoundedSolid*
Tube::createWasherLamina(double ri, double re, double h, int n,
    bool withQuadrilaterals)
{
    PolyhedralBoundedSolid* solid;

    solid = PolyhedralBoundedSolidEulerConstructions::createCircularLamina(
        0.0, 0.0, re, h, n);
    if ( ri <= 0.0 ) {
        if ( withQuadrilaterals ) {
            splitFaceIntoFan(solid, 1, Vector3Dd(0.0, 0.0, h));
            splitFaceIntoFan(solid, 2, Vector3Dd(0.0, 0.0, h));
            PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
        }
        return solid;
    }

    int firstInternal = solid->getMaxVertexId() + 1;
    PolyhedralBoundedSolidEulerOperators::smev(solid, 1, 1, firstInternal,
        Vector3Dd(ri, 0.0, h));
    PolyhedralBoundedSolidEulerOperators::kemr(solid, 1, 1,
        1, firstInternal, firstInternal, 1);
    // The internal circle goes clockwise (seen from +Z), against the
    // external one, so the disk it encloses is split as the new face
    PolyhedralBoundedSolidEulerConstructions::addArcToExistingFace(solid, 1,
        firstInternal, 0.0, 0.0, ri, h, 360.0, 360.0 / n, n - 1);
    int lastInternal = solid->getMaxVertexId();
    int diskFace = solid->getMaxFaceId() + 1;
    PolyhedralBoundedSolidEulerOperators::smef(solid, 1, lastInternal,
        firstInternal, diskFace);
    PolyhedralBoundedSolidEulerOperators::kfmrh(solid, 2, diskFace);
    if ( withQuadrilaterals ) {
        splitWasherIntoQuadrilaterals(solid, n);
    }
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
    return solid;
}

int Tube::splitFaceIntoFan(PolyhedralBoundedSolid* solid, int faceId,
    const Vector3Dd& center)
{
    _PolyhedralBoundedSolidFace* face = solid->findFace(faceId);
    if ( face == nullptr || face->boundariesList.size() <= 0 ) {
        return 0;
    }

    _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(0);
    _PolyhedralBoundedSolidHalfEdge* start = loop->boundaryStartHalfEdge;
    if ( start == nullptr ) {
        return 0;
    }

    java::ArrayList<int> ringVertexIds;
    _PolyhedralBoundedSolidHalfEdge* he = start;
    do {
        ringVertexIds.add(he->startingVertex->id);
        he = he->next();
    } while ( he != start );

    if ( ringVertexIds.size() < 3 ) {
        return 0;
    }

    int centerVertexId = solid->getMaxVertexId() + 1;
    PolyhedralBoundedSolidEulerOperators::smev(solid, faceId,
        ringVertexIds.get(0), centerVertexId, center);

    long int i;
    for ( i = 0; i < ringVertexIds.size() - 2; i++ ) {
        PolyhedralBoundedSolidEulerOperators::mef(solid, faceId, faceId,
            centerVertexId,
            ringVertexIds.get(i),
            ringVertexIds.get(i+1),
            ringVertexIds.get(i+2),
            solid->getMaxFaceId() + 1);
    }

    PolyhedralBoundedSolidEulerOperators::mef(solid, faceId, faceId,
        centerVertexId,
        ringVertexIds.get(ringVertexIds.size()-2),
        ringVertexIds.get(ringVertexIds.size()-1),
        ringVertexIds.get(0),
        solid->getMaxFaceId() + 1);
    return centerVertexId;
}

int Tube::findFaceWithVertices(PolyhedralBoundedSolid* solid,
    const java::ArrayList<int>& candidateFaceIds, int vertexId1,
    int vertexId2)
{
    long int i;
    long int j;

    for ( i = 0; i < candidateFaceIds.size(); i++ ) {
        _PolyhedralBoundedSolidFace* face =
            solid->findFace(candidateFaceIds.get(i));
        if ( face == nullptr ) {
            continue;
        }
        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(j);
            if ( loop->firstHalfEdgeAtVertex(vertexId1) != nullptr &&
                 loop->firstHalfEdgeAtVertex(vertexId2) != nullptr ) {
                return face->id;
            }
        }
    }
    return 0;
}

/**
The external circle has the vertices 1..n (vertex k+1 at k*360/n degrees)
and the internal one n+1..2n, clockwise: vertex n+1+j is at -j*360/n
degrees, so the external vertex k+1 pairs with the internal vertex
n+1+((n-k) mod n), at its same angle.
*/
void Tube::splitWasherIntoQuadrilaterals(PolyhedralBoundedSolid* solid,
    int n)
{
    int nextFaceId = 3;
    int side;
    int k;

    for ( side = 1; side <= 2; side++ ) {
        java::ArrayList<int> sideFaces;
        sideFaces.add(side);
        _PolyhedralBoundedSolidFace* face = solid->findFace(side);
        PolyhedralBoundedSolidEulerOperators::lmekr(solid,
            face->findHalfEdge(1), face->findHalfEdge(n + 1));
        for ( k = 1; k < n; k++ ) {
            int externalId = k + 1;
            int internalId = n + 1 + (n - k);
            int faceId = findFaceWithVertices(solid, sideFaces, externalId,
                internalId);
            PolyhedralBoundedSolidEulerOperators::smef(solid, faceId,
                externalId, internalId, nextFaceId);
            sideFaces.add(nextFaceId);
            nextFaceId++;
        }
    }
}

PolyhedralBoundedSolid*
Tube::buildPolyhedralBoundedSolid(int nsides)
{
    PolyhedralBoundedSolid* solid = createWasherLamina(internalRadius,
        externalRadius, 0.0, nsides);

    if ( !isWasher() ) {
        Matrix4x4d T = Matrix4x4d().translation(0.0, 0.0, height);
        PolyhedralBoundedSolidEulerConstructions::translationalSweepExtrudeFace(
            solid, solid->findFace(1), T);
    }
    return solid;
}

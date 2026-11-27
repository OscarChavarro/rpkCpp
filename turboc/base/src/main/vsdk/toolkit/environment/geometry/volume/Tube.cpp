//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#include <stddef.h>
#include <math.h>

#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayBoundary.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossing.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidBoundary.h"
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
    // Turbo C++ 3.0 crashes with the new of a template class as an argument
    MemberEntityControlAccessor<Tube, double> *accessor;
    accessor = new MemberEntityControlAccessor<Tube, double>(
        &Tube::getInternalRadius, &Tube::setInternalRadius);
    addControlSpecification("double;internalRadius;[0, INFINITE)", accessor);
    accessor = new MemberEntityControlAccessor<Tube, double>(
        &Tube::getExternalRadius, &Tube::setExternalRadius);
    addControlSpecification("double;externalRadius;(0, INFINITE)", accessor);
    accessor = new MemberEntityControlAccessor<Tube, double>(
        &Tube::getHeight, &Tube::setHeight);
    addControlSpecification("double;height;[0, INFINITE)", accessor);
}

double Tube::getInternalRadius() const
{
    return internalRadius;
}

double Tube::getExternalRadius() const
{
    return externalRadius;
}

double Tube::getHeight() const
{
    return height;
}

void Tube::setInternalRadius(double value)
{
    internalRadius = value;
}

void Tube::setExternalRadius(double value)
{
    externalRadius = value;
}

void Tube::setHeight(double value)
{
    height = value;
}

BOOLEAN Tube::isWasher() const
{
    return height <= 0.0;
}

Geometry* Tube::copyGeometry() const
{
    return new Tube(internalRadius, externalRadius, height);
}

BOOLEAN Tube::hasValidParameters() const
{
    return Double::isFinite(internalRadius) && Double::isFinite(externalRadius) &&
        Double::isFinite(height) && internalRadius >= 0 &&
        externalRadius > internalRadius && height >= 0;
}

//= Interior of the tube along a ray =====================================

BOOLEAN Tube::cylinderInterval(const Vector3Dd& origin,
    const Vector3Dd& direction, double radius, int surface, Interval& out)
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
            out = Interval(Double::NEGATIVE_INFINITY, SURFACE_NONE,
                Double::POSITIVE_INFINITY, SURFACE_NONE);
            return TRUE;
        }
        return FALSE;
    }
    double b = 2.0 * (ox * dx + oy * dy);
    double discriminant = b * b - 4.0 * a * c;

    if ( discriminant <= 0.0 ) {
        return FALSE;
    }
    double root = Math::sqrt(discriminant);
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
    return TRUE;
}

BOOLEAN Tube::slabInterval(const Vector3Dd& origin, const Vector3Dd& direction,
    Interval& out) const
{
    double oz = origin.z();
    double dz = direction.z();

    if ( dz == 0.0 ) {
        if ( oz > 0.0 && oz < height ) {
            out = Interval(Double::NEGATIVE_INFINITY, SURFACE_NONE,
                Double::POSITIVE_INFINITY, SURFACE_NONE);
            return TRUE;
        }
        return FALSE;
    }
    double tBottom = -oz / dz;
    double tTop = (height - oz) / dz;

    if ( tBottom < tTop ) {
        out = Interval(tBottom, SURFACE_BOTTOM, tTop, SURFACE_TOP);
        return TRUE;
    }
    out = Interval(tTop, SURFACE_TOP, tBottom, SURFACE_BOTTOM);
    return TRUE;
}

int Tube::computeInteriorIntervals(const Vector3Dd& origin,
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

    //- Slab intersected with the external cylinder ----------------------
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

    //- Minus the internal cylinder --------------------------------------
    Interval hole;
    BOOLEAN hasHole = FALSE;
    if ( internalRadius > 0.0 ) {
        hasHole = cylinderInterval(origin, direction, internalRadius,
            SURFACE_INTERNAL, hole);
    }
    if ( !hasHole || hole.end <= body.start || hole.start >= body.end ) {
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

double Tube::washerHit(const Vector3Dd& origin, const Vector3Dd& direction) const
{
    double dz = direction.z();

    if ( dz == 0.0 ) {
        return Double::NaN;
    }
    double t = -origin.z() / dz;
    double px = origin.x() + direction.x() * t;
    double py = origin.y() + direction.y() * t;
    double r2 = px * px + py * py;

    if ( r2 > externalRadius * externalRadius ||
         r2 < internalRadius * internalRadius ) {
        return Double::NaN;
    }
    return t;
}

double Tube::firstSurfaceHit(const Ray& ray, double minT, int surface[1]) const
{
    const Vector3Dd& origin = ray.getOrigin();
    const Vector3Dd& direction = ray.getDirection();

    if ( isWasher() ) {
        double t = washerHit(origin, direction);
        if ( !Double::isNaN(t) && t > minT ) {
            surface[0] = direction.z() < 0 ? (int)SURFACE_TOP : (int)SURFACE_BOTTOM;
            return t;
        }
        return Double::NaN;
    }

    Interval intervals[2];
    int n = computeInteriorIntervals(origin, direction, intervals);
    for ( int i = 0; i < n; i++ ) {
        if ( intervals[i].start > minT ) {
            surface[0] = intervals[i].startSurface;
            return intervals[i].start;
        }
        if ( intervals[i].end > minT ) {
            surface[0] = intervals[i].endSurface;
            return intervals[i].end;
        }
    }
    return Double::NaN;
}

//= Classic ray intersection interface ===================================

Ray* Tube::doIntersectionFirstHit(const Ray& inOutRay)
{
    RayHit hit;
    if ( doIntersectionFirstHit(inOutRay, &hit) && hit.getRay() != NULL ) {
        return new Ray(*hit.getRay());
    }
    return NULL;
}

BOOLEAN Tube::doIntersectionFirstHit(const Ray& inOutRay, RayHit* outHit)
{
    if ( !hasValidParameters() ) {
        return FALSE;
    }
    int surface[1] = {SURFACE_NONE};
    double t = firstSurfaceHit(inOutRay, VSDK::EPSILON, surface);

    if ( Double::isNaN(t) ) {
        return FALSE;
    }
    if ( outHit != NULL ) {
        if ( outHit->shouldStoreRay() || outHit->needsAnySurfaceData() ) {
            outHit->setRay(inOutRay.withT(t));
        }
        else {
            outHit->setHitDistance(t);
        }
        if ( outHit->needsAnySurfaceData() ) {
            fillSurfaceData(inOutRay, t, surface[0], outHit);
        }
    }
    return TRUE;
}

void Tube::doExtraInformation(const Ray& inRay, double inT, RayHit* outData)
{
    if ( outData == NULL ) {
        return;
    }
    Vector3Dd p = inRay.getOrigin().add(inRay.getDirection().multiply(inT));
    fillSurfaceData(inRay, inT, surfaceAt(p, inRay.getDirection()), outData);
}

int Tube::surfaceAt(const Vector3Dd& p, const Vector3Dd& rayDirection) const
{
    if ( isWasher() ) {
        return rayDirection.z() < 0 ? (int)SURFACE_TOP : (int)SURFACE_BOTTOM;
    }
    double rho = Math::sqrt(p.x() * p.x() + p.y() * p.y());
    double dBottom = Math::abs(p.z());
    double dTop = Math::abs(p.z() - height);
    double dExternal = Math::abs(rho - externalRadius);
    double dInternal = internalRadius > 0 ?
        Math::abs(rho - internalRadius) : Double::POSITIVE_INFINITY;
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

void Tube::fillSurfaceData(const Ray& ray, double t, int surface,
    RayHit* outData) const
{
    outData->beginDetailEvaluation();
    Vector3Dd p = ray.getOrigin().add(ray.getDirection().multiply(t));
    double rho = Math::sqrt(p.x() * p.x() + p.y() * p.y());
    double angle = atan2(p.y(), p.x());
    if ( angle < 0 ) {
        angle += 2 * Math::PI;
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
        outData->u = angle / (2 * Math::PI);
        if ( surface == SURFACE_TOP || surface == SURFACE_BOTTOM ) {
            outData->v = (rho - internalRadius) / (externalRadius - internalRadius);
        }
        else {
            outData->v = height > 0 ? p.z() / height : 0;
        }
        outData->markDetailAvailable(RayHit::DETAIL_UV);
    }
}

//= Ray crossing and classification interface ============================

RayFirstHitStatus::Value Tube::validateQuery(const Ray& ray,
    const RayCrossingQuery& query) const
{
    if ( Double::isNaN(query.minT) || Double::isNaN(query.maxT) ||
         query.minT > query.maxT ||
         !Double::isFinite(query.minimumChordSquared) ||
         query.minimumChordSquared < 0 ||
         Double::isNaN(query.minimumExteriorApproach) ) {
        return RayFirstHitStatus::InvalidInput;
    }
    if ( query.minimumChordSquared != 0 ||
         query.minimumExteriorApproach != Double::NEGATIVE_INFINITY ) {
        return RayFirstHitStatus::Unsupported;
    }
    const Vector3Dd& o = ray.getOrigin();
    const Vector3Dd& d = ray.getDirection();
    if ( !Double::isFinite(o.x()) || !Double::isFinite(o.y()) ||
         !Double::isFinite(o.z()) || !Double::isFinite(d.x()) ||
         !Double::isFinite(d.y()) || !Double::isFinite(d.z()) ||
         (d.x() == 0 && d.y() == 0 && d.z() == 0) ) {
        return RayFirstHitStatus::InvalidInput;
    }
    if ( !hasValidParameters() ) {
        return RayFirstHitStatus::InvalidInput;
    }
    return RayFirstHitStatus::Hit;
}

RayCrossingSupport::Value Tube::emitRayCrossings(const Ray& ray,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    RayFirstHitStatus::Value invalid = validateQuery(ray, query);
    if ( invalid == RayFirstHitStatus::Unsupported ) {
        return RayCrossingSupport::Unsupported;
    }
    if ( invalid != RayFirstHitStatus::Hit ) {
        return RayCrossingSupport::InvalidInput;
    }

    if ( isWasher() ) {
        double t = washerHit(ray.getOrigin(), ray.getDirection());
        if ( !Double::isNaN(t) && t >= query.minT && t <= query.maxT ) {
            sink.crossing(RayCrossing(t, this, RayCrossingKind::Tangent));
        }
        return RayCrossingSupport::Supported;
    }

    Interval intervals[2];
    int n = computeInteriorIntervals(ray.getOrigin(), ray.getDirection(),
        intervals);
    int i;
    for ( i = 0; i < n; i++ ) {
        if ( !Double::isFinite(intervals[i].start) ||
             !Double::isFinite(intervals[i].end) ) {
            return RayCrossingSupport::Indeterminate;
        }
    }
    for ( i = 0; i < n; i++ ) {
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

RayFirstHitStatus::Value Tube::queryFirstHit(const Ray& ray,
    const RayCrossingQuery& query, RayHit* result, RayBoundary* boundary) const
{
    if ( result != NULL ) {
        result->clear();
    }
    if ( boundary != NULL ) {
        *boundary = RayBoundary();
    }
    RayFirstHitStatus::Value invalid = validateQuery(ray, query);
    if ( invalid != RayFirstHitStatus::Hit ) {
        return invalid;
    }

    // The first boundary at or after minT: the surface hit is searched
    // strictly after a parameter slightly smaller than minT
    int surface[1] = {SURFACE_NONE};
    double t = firstSurfaceHit(ray,
        Math::nextAfter(query.minT, Double::NEGATIVE_INFINITY), surface);

    if ( Double::isNaN(t) || t > query.maxT ) {
        return RayFirstHitStatus::Miss;
    }
    if ( boundary != NULL ) {
        *boundary = RayBoundary(t, this);
    }
    if ( result != NULL ) {
        result->setHitDistance(t);
        if ( result->shouldStoreRay() ) {
            result->setRay(ray.withT(t));
        }
        if ( result->needsAnySurfaceData() ) {
            fillSurfaceData(ray, t, surface[0], result);
        }
    }
    return RayFirstHitStatus::Hit;
}

RayFirstHitStatus::Value Tube::querySurfaceDetail(const Ray& ray,
    const RayBoundary& boundary, RayHit* hit) const
{
    if ( hit != NULL ) {
        hit->clear();
    }
    if ( hit == NULL || boundary.geometry != this ||
         !Double::isFinite(boundary.t) ) {
        return RayFirstHitStatus::InvalidInput;
    }
    if ( boundary.getInstanceCount() != 0 ) {
        return RayFirstHitStatus::Unsupported;
    }
    RayFirstHitStatus::Value status = queryFirstHit(ray,
        RayCrossingQuery(boundary.t, boundary.t), NULL);
    if ( status != RayFirstHitStatus::Hit ) {
        return status;
    }
    hit->setHitDistance(boundary.t);
    if ( hit->shouldStoreRay() ) {
        hit->setRay(ray.withT(boundary.t));
    }
    ((Tube*)this)->doExtraInformation(ray, boundary.t, hit);
    if ( boundary.normalReversed && hit->needsNormal() &&
         (hit->getAvailableDetailMask() & RayHit::DETAIL_NORMAL) != 0 ) {
        hit->normal = hit->normal.multiply(-1);
    }
    return RayFirstHitStatus::Hit;
}

RaySolidQueryStatus::Value Tube::classifyRay(const Ray& ray,
    const RayCrossingQuery& query, RaySolidClassification& result) const
{
    result.clear();
    if ( query.minT >= query.maxT ) {
        return RaySolidQueryStatus::InvalidInput;
    }
    RayFirstHitStatus::Value invalid = validateQuery(ray, query);
    if ( invalid == RayFirstHitStatus::Unsupported ) {
        return RaySolidQueryStatus::Unsupported;
    }
    if ( invalid != RayFirstHitStatus::Hit ) {
        return RaySolidQueryStatus::InvalidInput;
    }

    Interval intervals[2];
    int n = computeInteriorIntervals(ray.getOrigin(), ray.getDirection(),
        intervals);
    BOOLEAN startsInside = FALSE;
    int i;

    for ( i = 0; i < n; i++ ) {
        if ( query.minT > intervals[i].start && query.minT <= intervals[i].end ) {
            startsInside = TRUE;
        }
    }
    result.insideSegments.add(startsInside);
    for ( i = 0; i < n; i++ ) {
        double entry = intervals[i].start;
        double exit = intervals[i].end;
        if ( entry >= query.minT && entry <= query.maxT ) {
            result.boundaries.add(RaySolidBoundary(entry, this));
            result.insideSegments.add(TRUE);
        }
        if ( exit >= query.minT && exit <= query.maxT ) {
            result.boundaries.add(RaySolidBoundary(exit, this));
            result.insideSegments.add(FALSE);
        }
    }
    return RaySolidQueryStatus::Supported;
}

//= Containment ==========================================================

int Tube::doContainmentTest(const Vector3Dd& p, double distanceTolerance)
{
    if ( !hasValidParameters() ) {
        return OUTSIDE;
    }
    double z = p.z();
    double rho = Math::sqrt(p.x() * p.x() + p.y() * p.y());

    if ( z < -distanceTolerance || z > height + distanceTolerance ||
         rho > externalRadius + distanceTolerance ||
         rho < internalRadius - distanceTolerance ) {
        return OUTSIDE;
    }
    BOOLEAN insideHole = internalRadius > 0 && rho < internalRadius;
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

GeometryContainment::Value Tube::classifyPoint(const Vector3Dd& p,
    double distanceTolerance) const
{
    if ( !Double::isFinite(distanceTolerance) || distanceTolerance < 0 ||
         !Double::isFinite(p.x()) || !Double::isFinite(p.y()) ||
         !Double::isFinite(p.z()) ) {
        return GeometryContainment::InvalidInput;
    }
    if ( !hasValidParameters() ) {
        return GeometryContainment::InvalidInput;
    }
    int c = ((Tube*)this)->doContainmentTest(p, distanceTolerance);
    if ( c == INSIDE ) {
        return GeometryContainment::Inside;
    }
    if ( c == LIMIT ) {
        return GeometryContainment::Boundary;
    }
    return GeometryContainment::Outside;
}

//= Bounds ===============================================================

void Tube::getBounds(GeometryBounds& result) const
{
    if ( !hasValidParameters() ) {
        result.setIndeterminate();
        return;
    }
    result.setFinite(Vector3Dd(-externalRadius, -externalRadius, 0),
        Vector3Dd(externalRadius, externalRadius, height));
}

double* Tube::getMinMax()
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

PolyhedralBoundedSolid* Tube::exportToPolyhedralBoundedSolid()
{
    return buildPolyhedralBoundedSolid(DEFAULT_CIRCUMFERENCE_DIVISIONS);
}

PolyhedralBoundedSolid* Tube::exportToPolyhedralBoundedSolid(
    int circumferenceDivisions)
{
    return buildPolyhedralBoundedSolid(
        Math::max((int)MIN_CIRCUMFERENCE_DIVISIONS, circumferenceDivisions));
}

PolyhedralBoundedSolid* Tube::createWasherLamina(double ri, double re,
    double h, int n, BOOLEAN withQuadrilaterals)
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
    if ( face == NULL || face->boundariesList.size() <= 0 ) {
        return 0;
    }

    _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(0);
    _PolyhedralBoundedSolidHalfEdge* start = loop->boundaryStartHalfEdge;
    if ( start == NULL ) {
        return 0;
    }

    ArrayList<int> ringVertexIds;
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
    const ArrayList<int>& candidateFaceIds, int vertexId1,
    int vertexId2)
{
    long int i;
    long int j;

    for ( i = 0; i < candidateFaceIds.size(); i++ ) {
        _PolyhedralBoundedSolidFace* face =
            solid->findFace(candidateFaceIds.get(i));
        if ( face == NULL ) {
            continue;
        }
        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(j);
            if ( loop->firstHalfEdgeAtVertex(vertexId1) != NULL &&
                 loop->firstHalfEdgeAtVertex(vertexId2) != NULL ) {
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
        ArrayList<int> sideFaces;
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

PolyhedralBoundedSolid* Tube::buildPolyhedralBoundedSolid(int nsides)
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

const Class Tube::CLASS = { "Tube", &Solid::CLASS };

const Class *
Tube::getClass() const
{
    return &CLASS;
}

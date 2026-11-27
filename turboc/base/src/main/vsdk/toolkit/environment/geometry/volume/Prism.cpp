//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =
//= [ROTH1982] Roth, Scott D. "Ray Casting for Modeling Solids", Computer  =
//=     Graphics and Image Processing 18, 109-144, 1982.                    =

#include <stddef.h>
#include <math.h>

#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/EntityControlAccessor.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayBoundary.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossing.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/volume/Prism.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerConstructions.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"

//= Construction and parameters ==========================================

Prism::Prism(int numberOfSides, double circumradius, double height) :
    height(height), validBase(FALSE), minX(0), minY(0), maxX(0), maxY(0)
{
    int i;
    int n = numberOfSides < 3 ? 3 : numberOfSides;
    ArrayList<Vector2Dd> polygon;
    for ( i = 0; i < n; i++ ) {
        double angle = 2.0 * Math::PI * ((double)i) / ((double)n);
        polygon.add(Vector2Dd(circumradius * cos(angle),
            circumradius * sin(angle)));
    }
    setBase(polygon);
    addControlSpecifications();
}

Prism::Prism(const ArrayList<Vector2Dd>& base, double height) :
    height(height), validBase(FALSE), minX(0), minY(0), maxX(0), maxY(0)
{
    setBase(base);
    addControlSpecifications();
}

void
Prism::addControlSpecifications()
{
    // Turbo C++ 3.0 crashes with the new of a template class as an argument
    MemberEntityControlAccessor<Prism, int> *sidesAccessor;
    MemberEntityControlAccessor<Prism, double> *accessor;
    sidesAccessor = new MemberEntityControlAccessor<Prism, int>(
        &Prism::getNumberOfSides, &Prism::setNumberOfSides);
    addControlSpecification("int;numberOfSides;[3, INFINITE)", sidesAccessor);
    accessor = new MemberEntityControlAccessor<Prism, double>(
        &Prism::getCircumradius, &Prism::setCircumradius);
    addControlSpecification("double;circumradius;(0, INFINITE)", accessor);
    accessor = new MemberEntityControlAccessor<Prism, double>(
        &Prism::getHeight, &Prism::setHeight);
    addControlSpecification("double;height;(0, INFINITE)", accessor);
}

Geometry*
Prism::copyGeometry() const
{
    return new Prism(base, height);
}

const ArrayList<Vector2Dd>&
Prism::getBase() const
{
    return base;
}

void
Prism::setBase(const ArrayList<Vector2Dd>& value)
{
    long int i;
    // Shoelace formula: a clockwise polygon has negative area
    double twiceArea = 0.0;
    long int n = value.size();
    for ( i = 0; i < n; i++ ) {
        const Vector2Dd& a = value[i];
        const Vector2Dd& b = value[(i + 1) % n];
        twiceArea += a.x * b.y - b.x * a.y;
    }
    base.clear();
    if ( twiceArea < 0.0 ) {
        for ( i = n - 1; i >= 0; i-- ) {
            base.add(value[i]);
        }
    }
    else {
        for ( i = 0; i < n; i++ ) {
            base.add(value[i]);
        }
    }
    updateBaseCache();
}

double
Prism::getHeight() const
{
    return height;
}

void
Prism::setHeight(double value)
{
    height = value;
}

int
Prism::getNumberOfSides() const
{
    return (int)base.size();
}

void
Prism::setNumberOfSides(int value)
{
    int i;
    int n = value < 3 ? 3 : value;
    double r = getCircumradius();
    if ( !(r > 0.0) || !Double::isFinite(r) ) {
        r = 1.0;
    }
    ArrayList<Vector2Dd> polygon;
    for ( i = 0; i < n; i++ ) {
        double angle = 2.0 * Math::PI * ((double)i) / ((double)n);
        polygon.add(Vector2Dd(r * cos(angle), r * sin(angle)));
    }
    setBase(polygon);
}

double
Prism::getCircumradius() const
{
    long int i;
    double r = 0.0;
    for ( i = 0; i < base.size(); i++ ) {
        double d = base[i].length();
        if ( d > r ) {
            r = d;
        }
    }
    return r;
}

void
Prism::setCircumradius(double value)
{
    long int i;
    double r = getCircumradius();
    if ( !(r > 0.0) ) {
        return;
    }
    double factor = value / r;
    ArrayList<Vector2Dd> polygon;
    for ( i = 0; i < base.size(); i++ ) {
        polygon.add(base[i].multiply(factor));
    }
    setBase(polygon);
}

double
Prism::cross(double ax, double ay, double bx, double by)
{
    return ax * by - ay * bx;
}

/**
@return true if the closed segments ab and cd share a point
*/
BOOLEAN
Prism::segmentsIntersect(const Vector2Dd& a, const Vector2Dd& b,
    const Vector2Dd& c, const Vector2Dd& d)
{
    double d1 = cross(b.x - a.x, b.y - a.y, c.x - a.x, c.y - a.y);
    double d2 = cross(b.x - a.x, b.y - a.y, d.x - a.x, d.y - a.y);
    double d3 = cross(d.x - c.x, d.y - c.y, a.x - c.x, a.y - c.y);
    double d4 = cross(d.x - c.x, d.y - c.y, b.x - c.x, b.y - c.y);

    if ( ((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
         ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)) ) {
        return TRUE;
    }
    // Collinear cases: an endpoint on the other segment
    double minX1 = Math::min(a.x, b.x);
    double maxX1 = Math::max(a.x, b.x);
    double minY1 = Math::min(a.y, b.y);
    double maxY1 = Math::max(a.y, b.y);
    double minX2 = Math::min(c.x, d.x);
    double maxX2 = Math::max(c.x, d.x);
    double minY2 = Math::min(c.y, d.y);
    double maxY2 = Math::max(c.y, d.y);
    if ( d1 == 0 && c.x >= minX1 && c.x <= maxX1 && c.y >= minY1 && c.y <= maxY1 ) {
        return TRUE;
    }
    if ( d2 == 0 && d.x >= minX1 && d.x <= maxX1 && d.y >= minY1 && d.y <= maxY1 ) {
        return TRUE;
    }
    if ( d3 == 0 && a.x >= minX2 && a.x <= maxX2 && a.y >= minY2 && a.y <= maxY2 ) {
        return TRUE;
    }
    if ( d4 == 0 && b.x >= minX2 && b.x <= maxX2 && b.y >= minY2 && b.y <= maxY2 ) {
        return TRUE;
    }
    return FALSE;
}

void
Prism::updateBaseCache()
{
    long int i;
    long int j;
    long int n = base.size();
    validBase = n >= 3;
    minX = minY = Double::POSITIVE_INFINITY;
    maxX = maxY = Double::NEGATIVE_INFINITY;
    for ( i = 0; i < n; i++ ) {
        const Vector2Dd& p = base[i];
        if ( !Double::isFinite(p.x) || !Double::isFinite(p.y) ) {
            validBase = FALSE;
        }
        minX = Math::min(minX, p.x);
        minY = Math::min(minY, p.y);
        maxX = Math::max(maxX, p.x);
        maxY = Math::max(maxY, p.y);
    }
    if ( !validBase ) {
        return;
    }

    // Non-zero area (setBase made it counterclockwise)
    double twiceArea = 0.0;
    for ( i = 0; i < n; i++ ) {
        const Vector2Dd& a = base[i];
        const Vector2Dd& b = base[(i + 1) % n];
        twiceArea += a.x * b.y - b.x * a.y;
    }
    if ( !(twiceArea > 0.0) ) {
        validBase = FALSE;
        return;
    }

    // Simple polygon: no repeated consecutive vertices, adjacent edges
    // only share their common vertex, other edges share nothing
    for ( i = 0; i < n && validBase; i++ ) {
        const Vector2Dd& a = base[i];
        const Vector2Dd& b = base[(i + 1) % n];
        if ( a == b ) {
            validBase = FALSE;
            break;
        }
        for ( j = i + 1; j < n; j++ ) {
            const Vector2Dd& c = base[j];
            const Vector2Dd& d = base[(j + 1) % n];
            BOOLEAN adjacent = j == i + 1 || (i == 0 && j == n - 1);
            if ( adjacent ) {
                // Shared vertex: they may not fold back over each other
                const Vector2Dd& shared = (j == i + 1) ? b : a;
                const Vector2Dd& other1 = (j == i + 1) ? a : b;
                const Vector2Dd& other2 = (j == i + 1) ? d : c;
                double side = cross(other1.x - shared.x, other1.y - shared.y,
                    other2.x - shared.x, other2.y - shared.y);
                double dot = (other1.x - shared.x) * (other2.x - shared.x) +
                    (other1.y - shared.y) * (other2.y - shared.y);
                if ( side == 0 && dot > 0 ) {
                    validBase = FALSE;
                    break;
                }
            }
            else if ( segmentsIntersect(a, b, c, d) ) {
                validBase = FALSE;
                break;
            }
        }
    }
}

BOOLEAN
Prism::hasValidParameters() const
{
    return validBase && Double::isFinite(height) && height > 0.0;
}

int
Prism::triangulateBase(ArrayList<int>& outTriangles) const
{
    long int i;
    long int j;
    outTriangles.clear();
    if ( !validBase ) {
        return 0;
    }
    ArrayList<int> remaining;
    for ( i = 0; i < base.size(); i++ ) {
        remaining.add((int)i);
    }
    int triangles = 0;
    while ( remaining.size() > 3 ) {
        long int m = remaining.size();
        BOOLEAN clipped = FALSE;
        for ( i = 0; i < m && !clipped; i++ ) {
            const Vector2Dd& a = base[remaining[(i + m - 1) % m]];
            const Vector2Dd& b = base[remaining[i]];
            const Vector2Dd& c = base[remaining[(i + 1) % m]];
            // Convex corner of a counterclockwise polygon
            if ( !(cross(b.x - a.x, b.y - a.y, c.x - b.x, c.y - b.y) > 0) ) {
                continue;
            }
            BOOLEAN ear = TRUE;
            for ( j = 0; j < m && ear; j++ ) {
                if ( j == i || j == (i + m - 1) % m || j == (i + 1) % m ) {
                    continue;
                }
                const Vector2Dd& p = base[remaining[j]];
                if ( p == a || p == b || p == c ) {
                    continue;
                }
                double s1 = cross(b.x - a.x, b.y - a.y, p.x - a.x, p.y - a.y);
                double s2 = cross(c.x - b.x, c.y - b.y, p.x - b.x, p.y - b.y);
                double s3 = cross(a.x - c.x, a.y - c.y, p.x - c.x, p.y - c.y);
                if ( s1 >= 0 && s2 >= 0 && s3 >= 0 ) {
                    ear = FALSE;
                }
            }
            if ( ear ) {
                outTriangles.add(remaining[(i + m - 1) % m]);
                outTriangles.add(remaining[i]);
                outTriangles.add(remaining[(i + 1) % m]);
                remaining.remove(i);
                triangles++;
                clipped = TRUE;
            }
        }
        if ( !clipped ) {
            // Only possible with rounding on nearly degenerate bases
            outTriangles.clear();
            return 0;
        }
    }
    outTriangles.add(remaining[0]);
    outTriangles.add(remaining[1]);
    outTriangles.add(remaining[2]);
    return triangles + 1;
}

//= Interior of the prism along a line ===================================

double
Prism::distanceToBaseBoundary(double x, double y, int& closestEdge) const
{
    long int i;
    double best = Double::POSITIVE_INFINITY;
    long int n = base.size();
    closestEdge = 0;
    for ( i = 0; i < n; i++ ) {
        const Vector2Dd& a = base[i];
        const Vector2Dd& b = base[(i + 1) % n];
        double ex = b.x - a.x;
        double ey = b.y - a.y;
        double lengthSquared = ex * ex + ey * ey;
        double s = lengthSquared > 0 ?
            ((x - a.x) * ex + (y - a.y) * ey) / lengthSquared : 0.0;
        if ( s < 0 ) {
            s = 0;
        }
        else if ( s > 1 ) {
            s = 1;
        }
        double dx = x - (a.x + s * ex);
        double dy = y - (a.y + s * ey);
        double d = Math::sqrt(dx * dx + dy * dy);
        if ( d < best ) {
            best = d;
            closestEdge = (int)i;
        }
    }
    return best;
}

int
Prism::classifyBasePoint(double x, double y) const
{
    long int i;
    long int n = base.size();
    BOOLEAN inside = FALSE;
    for ( i = 0; i < n; i++ ) {
        const Vector2Dd& a = base[i];
        const Vector2Dd& b = base[(i + 1) % n];
        double side = cross(b.x - a.x, b.y - a.y, x - a.x, y - a.y);
        if ( side == 0 &&
             x >= Math::min(a.x, b.x) && x <= Math::max(a.x, b.x) &&
             y >= Math::min(a.y, b.y) && y <= Math::max(a.y, b.y) ) {
            return 0;
        }
        // Crossing number with the half-open rule on y
        if ( (a.y > y) != (b.y > y) ) {
            double xCross = a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y);
            if ( xCross > x ) {
                inside = !inside;
            }
        }
    }
    return inside ? 1 : -1;
}

BOOLEAN
Prism::computeInteriorIntervals(const Vector3Dd& origin,
    const Vector3Dd& direction, ArrayList<Interval>& out) const
{
    long int i;
    out.clear();
    BOOLEAN welldefined = TRUE;

    //- Slab 0 <= z <= height ------------------------------------------
    Interval slab;
    BOOLEAN slabOnCap = FALSE;
    double oz = origin.z();
    double dz = direction.z();
    if ( dz == 0.0 ) {
        if ( oz == 0.0 || oz == height ) {
            slabOnCap = TRUE;
        }
        else if ( !(oz > 0.0 && oz < height) ) {
            return TRUE;
        }
        slab = Interval(Double::NEGATIVE_INFINITY, SURFACE_NONE,
            Double::POSITIVE_INFINITY, SURFACE_NONE);
    }
    else {
        double tBottom = -oz / dz;
        double tTop = (height - oz) / dz;
        if ( tBottom < tTop ) {
            slab = Interval(tBottom, SURFACE_BOTTOM, tTop, SURFACE_TOP);
        }
        else {
            slab = Interval(tTop, SURFACE_TOP, tBottom, SURFACE_BOTTOM);
        }
    }

    //- Parts of the projected line inside the base --------------------
    ArrayList<Interval> planar;
    double ox = origin.x();
    double oy = origin.y();
    double dx = direction.x();
    double dy = direction.y();
    long int n = base.size();

    if ( dx == 0.0 && dy == 0.0 ) {
        // Parallel to the walls: everything or nothing
        int c = classifyBasePoint(ox, oy);
        if ( c == 0 ) {
            return FALSE;
        }
        if ( c > 0 ) {
            planar.add(Interval(Double::NEGATIVE_INFINITY, SURFACE_NONE,
                Double::POSITIVE_INFINITY, SURFACE_NONE));
        }
    }
    else {
        // Crossings with the edges. A vertex on the line is taken as if it
        // were displaced to the positive side of the line (simulation of
        // simplicity), so the crossings always pair as entering / leaving.
        ArrayList<EdgeCrossing> crossings;
        for ( i = 0; i < n; i++ ) {
            const Vector2Dd& a = base[i];
            const Vector2Dd& b = base[(i + 1) % n];
            double sideA = cross(dx, dy, a.x - ox, a.y - oy);
            double sideB = cross(dx, dy, b.x - ox, b.y - oy);
            if ( sideA == 0.0 && sideB == 0.0 ) {
                // The edge is on the line: the line (which crosses the
                // slab) runs along its wall
                welldefined = FALSE;
                continue;
            }
            if ( (sideA < 0.0) == (sideB < 0.0) ) {
                continue;
            }
            double ex = b.x - a.x;
            double ey = b.y - a.y;
            double t = cross(a.x - ox, a.y - oy, ex, ey) / cross(dx, dy, ex, ey);
            // Insertion in increasing order of t
            long int position = crossings.size();
            crossings.add(EdgeCrossing(t, (int)i));
            while ( position > 0 && crossings[position - 1].lineParameter > t ) {
                crossings[position] = crossings[position - 1];
                position--;
            }
            crossings[position] = EdgeCrossing(t, (int)i);
        }
        for ( i = 0; i + 1 < crossings.size(); i += 2 ) {
            double start = crossings[i].lineParameter;
            double end = crossings[i + 1].lineParameter;
            if ( !(start < end) ) {
                // The line only touches a vertex
                continue;
            }
            int startSurface = SURFACE_FIRST_WALL + crossings[i].edge;
            int endSurface = SURFACE_FIRST_WALL + crossings[i + 1].edge;
            if ( planar.size() > 0 &&
                 planar[planar.size() - 1].end == start ) {
                // The line goes through a reflex vertex: no boundary there
                planar[planar.size() - 1].end = end;
                planar[planar.size() - 1].endSurface = endSurface;
            }
            else {
                planar.add(Interval(start, startSurface, end, endSurface));
            }
        }
    }

    //- Slab intersected with the planar intervals ---------------------
    for ( i = 0; i < planar.size(); i++ ) {
        Interval body = planar[i];
        if ( slab.start > body.start ) {
            body.start = slab.start;
            body.startSurface = slab.startSurface;
        }
        if ( slab.end < body.end ) {
            body.end = slab.end;
            body.endSurface = slab.endSurface;
        }
        if ( body.start < body.end ) {
            if ( slabOnCap ) {
                // The line lies on the plane of a cap, over the base
                welldefined = FALSE;
            }
            out.add(body);
        }
    }
    if ( slabOnCap ) {
        // A line on a cap only touches the prism
        out.clear();
    }
    return welldefined;
}

double
Prism::firstSurfaceHit(const Ray& ray, double minT, int& surface,
    BOOLEAN& overlap) const
{
    long int i;
    ArrayList<Interval> intervals;
    overlap = !computeInteriorIntervals(ray.getOrigin(), ray.getDirection(),
        intervals);
    for ( i = 0; i < intervals.size(); i++ ) {
        if ( intervals[i].start > minT ) {
            surface = intervals[i].startSurface;
            return intervals[i].start;
        }
        if ( intervals[i].end > minT ) {
            surface = intervals[i].endSurface;
            return intervals[i].end;
        }
    }
    return Double::NaN;
}

//= Classic ray intersection interface ===================================

Ray*
Prism::doIntersectionFirstHit(const Ray& inOutRay)
{
    RayHit hit;
    if ( doIntersectionFirstHit(inOutRay, &hit) && hit.getRay() != NULL ) {
        return new Ray(*hit.getRay());
    }
    return NULL;
}

BOOLEAN
Prism::doIntersectionFirstHit(const Ray& inOutRay, RayHit* outHit)
{
    if ( !hasValidParameters() ) {
        return FALSE;
    }
    int surface = SURFACE_NONE;
    BOOLEAN overlap = FALSE;
    double t = firstSurfaceHit(inOutRay, VSDK::EPSILON, surface, overlap);

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
            fillSurfaceData(inOutRay, t, surface, outHit);
        }
    }
    return TRUE;
}

void
Prism::doExtraInformation(const Ray& inRay, double inT, RayHit* outData)
{
    if ( outData == NULL ) {
        return;
    }
    Vector3Dd p = inRay.getOrigin().add(inRay.getDirection().multiply(inT));
    fillSurfaceData(inRay, inT, surfaceAt(p), outData);
}

int
Prism::surfaceAt(const Vector3Dd& p) const
{
    int edge = 0;
    double dWall = distanceToBaseBoundary(p.x(), p.y(), edge);
    double dBottom = Math::abs(p.z());
    double dTop = Math::abs(p.z() - height);
    int surface = SURFACE_BOTTOM;
    double best = dBottom;

    if ( dTop < best ) {
        best = dTop;
        surface = SURFACE_TOP;
    }
    if ( dWall < best ) {
        surface = SURFACE_FIRST_WALL + edge;
    }
    return surface;
}

void
Prism::fillSurfaceData(const Ray& ray, double t, int surface,
    RayHit* outData) const
{
    outData->beginDetailEvaluation();
    Vector3Dd p = ray.getOrigin().add(ray.getDirection().multiply(t));
    Vector3Dd normal;
    Vector3Dd tangent;
    double u;
    double v;
    double width = maxX - minX;
    double depth = maxY - minY;

    if ( surface == SURFACE_TOP || surface == SURFACE_BOTTOM ) {
        normal = Vector3Dd(0, 0, surface == SURFACE_TOP ? 1 : -1);
        tangent = Vector3Dd(1, 0, 0);
        u = width > 0 ? (p.x() - minX) / width : 0;
        v = depth > 0 ? (p.y() - minY) / depth : 0;
    }
    else {
        long int n = base.size();
        int edge = surface - SURFACE_FIRST_WALL;
        if ( edge < 0 || edge >= n ) {
            edge = 0;
        }
        const Vector2Dd& a = base[edge];
        const Vector2Dd& b = base[(edge + 1) % n];
        double ex = b.x - a.x;
        double ey = b.y - a.y;
        double length = Math::sqrt(ex * ex + ey * ey);
        // Counterclockwise base: the exterior is on the right of each edge
        normal = Vector3Dd(ey / length, -ex / length, 0);
        tangent = Vector3Dd(ex / length, ey / length, 0);
        double s = ((p.x() - a.x) * ex + (p.y() - a.y) * ey) /
            (length * length);
        u = (edge + s) / ((double)n);
        v = height > 0 ? p.z() / height : 0;
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
        outData->tangent = tangent;
        outData->markDetailAvailable(RayHit::DETAIL_TANGENT);
    }
    if ( outData->needsTextureCoordinates() ) {
        outData->u = u;
        outData->v = v;
        outData->markDetailAvailable(RayHit::DETAIL_UV);
    }
}

//= Ray crossing and classification interface ============================

BOOLEAN
Prism::validateQuery(const Ray& ray, const RayCrossingQuery& query,
    RayFirstHitStatus::Value& failure) const
{
    if ( Double::isNaN(query.minT) || Double::isNaN(query.maxT) ||
         query.minT > query.maxT ||
         !Double::isFinite(query.minimumChordSquared) ||
         query.minimumChordSquared < 0 ||
         Double::isNaN(query.minimumExteriorApproach) ) {
        failure = RayFirstHitStatus::InvalidInput;
        return TRUE;
    }
    if ( query.minimumChordSquared != 0 ||
         query.minimumExteriorApproach != Double::NEGATIVE_INFINITY ) {
        failure = RayFirstHitStatus::Unsupported;
        return TRUE;
    }
    Vector3Dd o = ray.getOrigin();
    Vector3Dd d = ray.getDirection();
    if ( !Double::isFinite(o.x()) || !Double::isFinite(o.y()) ||
         !Double::isFinite(o.z()) || !Double::isFinite(d.x()) ||
         !Double::isFinite(d.y()) || !Double::isFinite(d.z()) ||
         (d.x() == 0 && d.y() == 0 && d.z() == 0) ) {
        failure = RayFirstHitStatus::InvalidInput;
        return TRUE;
    }
    if ( !hasValidParameters() ) {
        failure = RayFirstHitStatus::InvalidInput;
        return TRUE;
    }
    return FALSE;
}

RayCrossingSupport::Value
Prism::emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
    RayCrossingSink& sink) const
{
    long int i;
    RayFirstHitStatus::Value invalid;
    if ( validateQuery(ray, query, invalid) ) {
        if ( invalid == RayFirstHitStatus::Unsupported ) {
            return RayCrossingSupport::Unsupported;
        }
        return RayCrossingSupport::InvalidInput;
    }

    ArrayList<Interval> intervals;
    if ( !computeInteriorIntervals(ray.getOrigin(), ray.getDirection(),
             intervals) ) {
        return RayCrossingSupport::Indeterminate;
    }
    for ( i = 0; i < intervals.size(); i++ ) {
        if ( !Double::isFinite(intervals[i].start) ||
             !Double::isFinite(intervals[i].end) ) {
            return RayCrossingSupport::Indeterminate;
        }
    }
    for ( i = 0; i < intervals.size(); i++ ) {
        double entry = intervals[i].start;
        double exit = intervals[i].end;
        if ( entry >= query.minT && entry <= query.maxT ) {
            RayCrossing crossing(entry, this, RayCrossingKind::Entering);
            crossing.surfaceIndex = intervals[i].startSurface;
            sink.crossing(crossing);
        }
        if ( exit >= query.minT && exit <= query.maxT ) {
            RayCrossing crossing(exit, this, RayCrossingKind::Leaving);
            crossing.surfaceIndex = intervals[i].endSurface;
            sink.crossing(crossing);
        }
    }
    return RayCrossingSupport::Supported;
}

RayFirstHitStatus::Value
Prism::queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
    RayHit* result, RayBoundary* boundary) const
{
    if ( result != NULL ) {
        result->clear();
    }
    if ( boundary != NULL ) {
        *boundary = RayBoundary();
    }
    RayFirstHitStatus::Value invalid;
    if ( validateQuery(ray, query, invalid) ) {
        return invalid;
    }

    // The first boundary at or after minT: the surface hit is searched
    // strictly after a parameter slightly smaller than minT
    int surface = SURFACE_NONE;
    BOOLEAN overlap = FALSE;
    double t = firstSurfaceHit(ray,
        Math::nextAfter(query.minT, Double::NEGATIVE_INFINITY),
        surface, overlap);

    if ( overlap ) {
        return RayFirstHitStatus::Indeterminate;
    }
    if ( Double::isNaN(t) || t > query.maxT ) {
        return RayFirstHitStatus::Miss;
    }
    if ( boundary != NULL ) {
        *boundary = RayBoundary(t, this, FALSE, surface);
    }
    if ( result != NULL ) {
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

RayFirstHitStatus::Value
Prism::querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
    RayHit* hit) const
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
    RayBoundary found;
    RayFirstHitStatus::Value status = queryFirstHit(ray,
        RayCrossingQuery(boundary.t, boundary.t), NULL, &found);
    if ( status != RayFirstHitStatus::Hit ) {
        return status;
    }
    hit->setHitDistance(boundary.t);
    if ( hit->shouldStoreRay() ) {
        hit->setRay(ray.withT(boundary.t));
    }
    int surface = boundary.surfaceIndex >= SURFACE_BOTTOM &&
        boundary.surfaceIndex < SURFACE_FIRST_WALL + base.size() ?
        (int)boundary.surfaceIndex : (int)found.surfaceIndex;
    fillSurfaceData(ray, boundary.t, surface, hit);
    if ( boundary.normalReversed && hit->needsNormal() &&
         (hit->getAvailableDetailMask() & RayHit::DETAIL_NORMAL) != 0 ) {
        hit->normal = hit->normal.multiply(-1);
    }
    return RayFirstHitStatus::Hit;
}

RaySolidQueryStatus::Value
Prism::classifyRay(const Ray& ray, const RayCrossingQuery& query,
    RaySolidClassification& result) const
{
    long int i;
    result.clear();
    if ( query.minT >= query.maxT ) {
        return RaySolidQueryStatus::InvalidInput;
    }
    RayFirstHitStatus::Value invalid;
    if ( validateQuery(ray, query, invalid) ) {
        if ( invalid == RayFirstHitStatus::Unsupported ) {
            return RaySolidQueryStatus::Unsupported;
        }
        return RaySolidQueryStatus::InvalidInput;
    }
    ArrayList<Interval> intervals;
    if ( !computeInteriorIntervals(ray.getOrigin(), ray.getDirection(),
             intervals) ) {
        return RaySolidQueryStatus::Indeterminate;
    }
    BOOLEAN startsInside = FALSE;

    for ( i = 0; i < intervals.size(); i++ ) {
        if ( query.minT > intervals[i].start &&
             query.minT <= intervals[i].end ) {
            startsInside = TRUE;
        }
    }
    if ( !result.insideSegments.add(startsInside) ) {
        return RaySolidQueryStatus::Indeterminate;
    }
    for ( i = 0; i < intervals.size(); i++ ) {
        double entry = intervals[i].start;
        double exit = intervals[i].end;
        if ( entry >= query.minT && entry <= query.maxT ) {
            if ( !result.boundaries.add(RaySolidBoundary(entry, this, FALSE,
                     intervals[i].startSurface)) ||
                 !result.insideSegments.add(TRUE) ) {
                result.clear();
                return RaySolidQueryStatus::Indeterminate;
            }
        }
        if ( exit >= query.minT && exit <= query.maxT ) {
            if ( !result.boundaries.add(RaySolidBoundary(exit, this, FALSE,
                     intervals[i].endSurface)) ||
                 !result.insideSegments.add(FALSE) ) {
                result.clear();
                return RaySolidQueryStatus::Indeterminate;
            }
        }
    }
    return RaySolidQueryStatus::Supported;
}

//= Containment ==========================================================

int
Prism::doContainmentTest(const Vector3Dd& p, double distanceTolerance)
{
    if ( !hasValidParameters() ) {
        return OUTSIDE;
    }
    double z = p.z();
    if ( z < -distanceTolerance || z > height + distanceTolerance ) {
        return OUTSIDE;
    }
    int edge = 0;
    double dWall = distanceToBaseBoundary(p.x(), p.y(), edge);
    int c = classifyBasePoint(p.x(), p.y());

    if ( c < 0 ) {
        return dWall <= distanceTolerance ? LIMIT : OUTSIDE;
    }
    if ( c > 0 && dWall > distanceTolerance && z > distanceTolerance &&
         z < height - distanceTolerance ) {
        return INSIDE;
    }
    return LIMIT;
}

GeometryContainment::Value
Prism::classifyPoint(const Vector3Dd& p, double distanceTolerance) const
{
    if ( !Double::isFinite(distanceTolerance) || distanceTolerance < 0 ||
         !Double::isFinite(p.x()) || !Double::isFinite(p.y()) ||
         !Double::isFinite(p.z()) ) {
        return GeometryContainment::InvalidInput;
    }
    if ( !hasValidParameters() ) {
        return GeometryContainment::InvalidInput;
    }
    int c = ((Prism*)this)->doContainmentTest(p, distanceTolerance);
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
Prism::getBounds(GeometryBounds& result) const
{
    if ( !hasValidParameters() ) {
        result.setIndeterminate();
        return;
    }
    result.setFinite(Vector3Dd(minX, minY, 0), Vector3Dd(maxX, maxY, height));
}

double*
Prism::getMinMax()
{
    double* minmax = new double[6];

    minmax[0] = minX;
    minmax[1] = minY;
    minmax[2] = 0;
    minmax[3] = maxX;
    minmax[4] = maxY;
    minmax[5] = height;
    return minmax;
}

//= Boundary representation ==============================================

PolyhedralBoundedSolid*
Prism::exportToPolyhedralBoundedSolid()
{
    return buildPolyhedralBoundedSolid();
}

PolyhedralBoundedSolid*
Prism::buildPolyhedralBoundedSolid()
{
    PolyhedralBoundedSolid* solid = new PolyhedralBoundedSolid();
    int n = (int)base.size();
    int i;

    PolyhedralBoundedSolidEulerOperators::mvfs(solid,
        Vector3Dd(base[0].x, base[0].y, 0.0), 1, 1);
    for ( i = 1; i < n; i++ ) {
        PolyhedralBoundedSolidEulerOperators::smev(solid, 1, i, i + 1,
            Vector3Dd(base[i].x, base[i].y, 0.0));
    }
    PolyhedralBoundedSolidEulerOperators::smef(solid, 1, n, 1, 2);
    Matrix4x4d t;
    t = t.translation(0.0, 0.0, height);
    PolyhedralBoundedSolidEulerConstructions::translationalSweepExtrudeFace(
        solid, solid->findFace(1), t);
    return solid;
}

const Class Prism::CLASS = { "Prism", &Solid::CLASS };

const Class *
Prism::getClass() const
{
    return &CLASS;
}

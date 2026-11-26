//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =
//= [ROTH1982] Roth, Scott D. "Ray Casting for Modeling Solids", Computer  =
//=     Graphics and Image Processing 18, 109-144, 1982.                    =

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
#include "vsdk/toolkit/environment/geometry/volume/Prism.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerConstructions.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"

//= Construction and parameters ==========================================

Prism::Prism(int numberOfSides, double circumradius, double height) :
    height(height), validBase(false), minX(0), minY(0), maxX(0), maxY(0)
{
    int n = numberOfSides < 3 ? 3 : numberOfSides;
    java::ArrayList<Vector2Dd> polygon;
    for ( int i = 0; i < n; i++ ) {
        double angle = 2.0 * java::Math::PI * ((double)i) / ((double)n);
        polygon.add(Vector2Dd(circumradius * std::cos(angle),
            circumradius * std::sin(angle)));
    }
    setBase(polygon);
    addControlSpecifications();
}

Prism::Prism(const java::ArrayList<Vector2Dd>& base, double height) :
    height(height), validBase(false), minX(0), minY(0), maxX(0), maxY(0)
{
    setBase(base);
    addControlSpecifications();
}

void
Prism::addControlSpecifications()
{
    addControlSpecification("int;numberOfSides;[3, INFINITE)",
        &Prism::getNumberOfSides, &Prism::setNumberOfSides);
    addControlSpecification("double;circumradius;(0, INFINITE)",
        &Prism::getCircumradius, &Prism::setCircumradius);
    addControlSpecification("double;height;(0, INFINITE)",
        &Prism::getHeight, &Prism::setHeight);
}

Geometry*
Prism::copyGeometry() const
{
    return new Prism(base, height);
}

const java::ArrayList<Vector2Dd>&
Prism::getBase() const
{
    return base;
}

void
Prism::setBase(const java::ArrayList<Vector2Dd>& value)
{
    // Shoelace formula: a clockwise polygon has negative area
    double twiceArea = 0.0;
    long int n = value.size();
    for ( long int i = 0; i < n; i++ ) {
        const Vector2Dd& a = value[i];
        const Vector2Dd& b = value[(i + 1) % n];
        twiceArea += a.x * b.y - b.x * a.y;
    }
    base.clear();
    if ( twiceArea < 0.0 ) {
        for ( long int i = n - 1; i >= 0; i-- ) {
            base.add(value[i]);
        }
    }
    else {
        for ( long int i = 0; i < n; i++ ) {
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
    int n = value < 3 ? 3 : value;
    double r = getCircumradius();
    if ( !(r > 0.0) || !java::Double::isFinite(r) ) {
        r = 1.0;
    }
    java::ArrayList<Vector2Dd> polygon;
    for ( int i = 0; i < n; i++ ) {
        double angle = 2.0 * java::Math::PI * ((double)i) / ((double)n);
        polygon.add(Vector2Dd(r * std::cos(angle), r * std::sin(angle)));
    }
    setBase(polygon);
}

double
Prism::getCircumradius() const
{
    double r = 0.0;
    for ( long int i = 0; i < base.size(); i++ ) {
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
    double r = getCircumradius();
    if ( !(r > 0.0) ) {
        return;
    }
    double factor = value / r;
    java::ArrayList<Vector2Dd> polygon;
    for ( long int i = 0; i < base.size(); i++ ) {
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
bool
Prism::segmentsIntersect(const Vector2Dd& a, const Vector2Dd& b,
    const Vector2Dd& c, const Vector2Dd& d)
{
    double d1 = cross(b.x - a.x, b.y - a.y, c.x - a.x, c.y - a.y);
    double d2 = cross(b.x - a.x, b.y - a.y, d.x - a.x, d.y - a.y);
    double d3 = cross(d.x - c.x, d.y - c.y, a.x - c.x, a.y - c.y);
    double d4 = cross(d.x - c.x, d.y - c.y, b.x - c.x, b.y - c.y);

    if ( ((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
         ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)) ) {
        return true;
    }
    // Collinear cases: an endpoint on the other segment
    double minX1 = java::Math::min(a.x, b.x);
    double maxX1 = java::Math::max(a.x, b.x);
    double minY1 = java::Math::min(a.y, b.y);
    double maxY1 = java::Math::max(a.y, b.y);
    double minX2 = java::Math::min(c.x, d.x);
    double maxX2 = java::Math::max(c.x, d.x);
    double minY2 = java::Math::min(c.y, d.y);
    double maxY2 = java::Math::max(c.y, d.y);
    if ( d1 == 0 && c.x >= minX1 && c.x <= maxX1 && c.y >= minY1 && c.y <= maxY1 ) {
        return true;
    }
    if ( d2 == 0 && d.x >= minX1 && d.x <= maxX1 && d.y >= minY1 && d.y <= maxY1 ) {
        return true;
    }
    if ( d3 == 0 && a.x >= minX2 && a.x <= maxX2 && a.y >= minY2 && a.y <= maxY2 ) {
        return true;
    }
    if ( d4 == 0 && b.x >= minX2 && b.x <= maxX2 && b.y >= minY2 && b.y <= maxY2 ) {
        return true;
    }
    return false;
}

void
Prism::updateBaseCache()
{
    long int n = base.size();
    validBase = n >= 3;
    minX = minY = java::Double::POSITIVE_INFINITY;
    maxX = maxY = java::Double::NEGATIVE_INFINITY;
    for ( long int i = 0; i < n; i++ ) {
        const Vector2Dd& p = base[i];
        if ( !java::Double::isFinite(p.x) || !java::Double::isFinite(p.y) ) {
            validBase = false;
        }
        minX = java::Math::min(minX, p.x);
        minY = java::Math::min(minY, p.y);
        maxX = java::Math::max(maxX, p.x);
        maxY = java::Math::max(maxY, p.y);
    }
    if ( !validBase ) {
        return;
    }

    // Non-zero area (setBase made it counterclockwise)
    double twiceArea = 0.0;
    for ( long int i = 0; i < n; i++ ) {
        const Vector2Dd& a = base[i];
        const Vector2Dd& b = base[(i + 1) % n];
        twiceArea += a.x * b.y - b.x * a.y;
    }
    if ( !(twiceArea > 0.0) ) {
        validBase = false;
        return;
    }

    // Simple polygon: no repeated consecutive vertices, adjacent edges
    // only share their common vertex, other edges share nothing
    for ( long int i = 0; i < n && validBase; i++ ) {
        const Vector2Dd& a = base[i];
        const Vector2Dd& b = base[(i + 1) % n];
        if ( a == b ) {
            validBase = false;
            break;
        }
        for ( long int j = i + 1; j < n; j++ ) {
            const Vector2Dd& c = base[j];
            const Vector2Dd& d = base[(j + 1) % n];
            bool adjacent = j == i + 1 || (i == 0 && j == n - 1);
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
                    validBase = false;
                    break;
                }
            }
            else if ( segmentsIntersect(a, b, c, d) ) {
                validBase = false;
                break;
            }
        }
    }
}

bool
Prism::hasValidParameters() const
{
    return validBase && java::Double::isFinite(height) && height > 0.0;
}

int
Prism::triangulateBase(java::ArrayList<int>& outTriangles) const
{
    outTriangles.clear();
    if ( !validBase ) {
        return 0;
    }
    java::ArrayList<int> remaining;
    for ( long int i = 0; i < base.size(); i++ ) {
        remaining.add((int)i);
    }
    int triangles = 0;
    while ( remaining.size() > 3 ) {
        long int m = remaining.size();
        bool clipped = false;
        for ( long int i = 0; i < m && !clipped; i++ ) {
            const Vector2Dd& a = base[remaining[(i + m - 1) % m]];
            const Vector2Dd& b = base[remaining[i]];
            const Vector2Dd& c = base[remaining[(i + 1) % m]];
            // Convex corner of a counterclockwise polygon
            if ( !(cross(b.x - a.x, b.y - a.y, c.x - b.x, c.y - b.y) > 0) ) {
                continue;
            }
            bool ear = true;
            for ( long int j = 0; j < m && ear; j++ ) {
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
                    ear = false;
                }
            }
            if ( ear ) {
                outTriangles.add(remaining[(i + m - 1) % m]);
                outTriangles.add(remaining[i]);
                outTriangles.add(remaining[(i + 1) % m]);
                remaining.remove(i);
                triangles++;
                clipped = true;
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
    double best = java::Double::POSITIVE_INFINITY;
    long int n = base.size();
    closestEdge = 0;
    for ( long int i = 0; i < n; i++ ) {
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
        double d = java::Math::sqrt(dx * dx + dy * dy);
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
    long int n = base.size();
    bool inside = false;
    for ( long int i = 0; i < n; i++ ) {
        const Vector2Dd& a = base[i];
        const Vector2Dd& b = base[(i + 1) % n];
        double side = cross(b.x - a.x, b.y - a.y, x - a.x, y - a.y);
        if ( side == 0 &&
             x >= java::Math::min(a.x, b.x) && x <= java::Math::max(a.x, b.x) &&
             y >= java::Math::min(a.y, b.y) && y <= java::Math::max(a.y, b.y) ) {
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

bool
Prism::computeInteriorIntervals(const Vector3Dd& origin,
    const Vector3Dd& direction, java::ArrayList<Interval>& out) const
{
    out.clear();
    bool welldefined = true;

    //- Slab 0 <= z <= height ------------------------------------------
    Interval slab;
    bool slabOnCap = false;
    double oz = origin.z();
    double dz = direction.z();
    if ( dz == 0.0 ) {
        if ( oz == 0.0 || oz == height ) {
            slabOnCap = true;
        }
        else if ( !(oz > 0.0 && oz < height) ) {
            return true;
        }
        slab = Interval(java::Double::NEGATIVE_INFINITY, SURFACE_NONE,
            java::Double::POSITIVE_INFINITY, SURFACE_NONE);
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
    java::ArrayList<Interval> planar;
    double ox = origin.x();
    double oy = origin.y();
    double dx = direction.x();
    double dy = direction.y();
    long int n = base.size();

    if ( dx == 0.0 && dy == 0.0 ) {
        // Parallel to the walls: everything or nothing
        int c = classifyBasePoint(ox, oy);
        if ( c == 0 ) {
            return false;
        }
        if ( c > 0 ) {
            planar.add(Interval(java::Double::NEGATIVE_INFINITY, SURFACE_NONE,
                java::Double::POSITIVE_INFINITY, SURFACE_NONE));
        }
    }
    else {
        // Crossings with the edges. A vertex on the line is taken as if it
        // were displaced to the positive side of the line (simulation of
        // simplicity), so the crossings always pair as entering / leaving.
        java::ArrayList<EdgeCrossing> crossings;
        for ( long int i = 0; i < n; i++ ) {
            const Vector2Dd& a = base[i];
            const Vector2Dd& b = base[(i + 1) % n];
            double sideA = cross(dx, dy, a.x - ox, a.y - oy);
            double sideB = cross(dx, dy, b.x - ox, b.y - oy);
            if ( sideA == 0.0 && sideB == 0.0 ) {
                // The edge is on the line: the line (which crosses the
                // slab) runs along its wall
                welldefined = false;
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
        for ( long int i = 0; i + 1 < crossings.size(); i += 2 ) {
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
    for ( long int i = 0; i < planar.size(); i++ ) {
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
                welldefined = false;
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
    bool& overlap) const
{
    java::ArrayList<Interval> intervals;
    overlap = !computeInteriorIntervals(ray.getOrigin(), ray.getDirection(),
        intervals);
    for ( long int i = 0; i < intervals.size(); i++ ) {
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
Prism::doIntersectionFirstHit(const Ray& inOutRay)
{
    RayHit hit;
    if ( doIntersectionFirstHit(inOutRay, &hit) && hit.getRay() != nullptr ) {
        return new Ray(*hit.getRay());
    }
    return nullptr;
}

bool
Prism::doIntersectionFirstHit(const Ray& inOutRay, RayHit* outHit)
{
    if ( !hasValidParameters() ) {
        return false;
    }
    int surface = SURFACE_NONE;
    bool overlap = false;
    double t = firstSurfaceHit(inOutRay, VSDK::EPSILON, surface, overlap);

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
Prism::doExtraInformation(const Ray& inRay, double inT, RayHit* outData)
{
    if ( outData == nullptr ) {
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
    double dBottom = java::Math::abs(p.z());
    double dTop = java::Math::abs(p.z() - height);
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
        double length = java::Math::sqrt(ex * ex + ey * ey);
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

bool
Prism::validateQuery(const Ray& ray, const RayCrossingQuery& query,
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
Prism::emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
    RayCrossingSink& sink) const
{
    RayFirstHitStatus invalid;
    if ( validateQuery(ray, query, invalid) ) {
        if ( invalid == RayFirstHitStatus::Unsupported ) {
            return RayCrossingSupport::Unsupported;
        }
        return RayCrossingSupport::InvalidInput;
    }

    java::ArrayList<Interval> intervals;
    if ( !computeInteriorIntervals(ray.getOrigin(), ray.getDirection(),
             intervals) ) {
        return RayCrossingSupport::Indeterminate;
    }
    for ( long int i = 0; i < intervals.size(); i++ ) {
        if ( !java::Double::isFinite(intervals[i].start) ||
             !java::Double::isFinite(intervals[i].end) ) {
            return RayCrossingSupport::Indeterminate;
        }
    }
    for ( long int i = 0; i < intervals.size(); i++ ) {
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

RayFirstHitStatus
Prism::queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
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
    bool overlap = false;
    double t = firstSurfaceHit(ray,
        java::Math::nextAfter(query.minT, java::Double::NEGATIVE_INFINITY),
        surface, overlap);

    if ( overlap ) {
        return RayFirstHitStatus::Indeterminate;
    }
    if ( java::Double::isNaN(t) || t > query.maxT ) {
        return RayFirstHitStatus::Miss;
    }
    if ( boundary != nullptr ) {
        *boundary = RayBoundary(t, this, false, surface);
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
Prism::querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
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
    RayBoundary found;
    RayFirstHitStatus status = queryFirstHit(ray,
        RayCrossingQuery(boundary.t, boundary.t), nullptr, &found);
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

RaySolidQueryStatus
Prism::classifyRay(const Ray& ray, const RayCrossingQuery& query,
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
    java::ArrayList<Interval> intervals;
    if ( !computeInteriorIntervals(ray.getOrigin(), ray.getDirection(),
             intervals) ) {
        return RaySolidQueryStatus::Indeterminate;
    }
    bool startsInside = false;

    for ( long int i = 0; i < intervals.size(); i++ ) {
        if ( query.minT > intervals[i].start &&
             query.minT <= intervals[i].end ) {
            startsInside = true;
        }
    }
    if ( !result.insideSegments.add(startsInside) ) {
        return RaySolidQueryStatus::Indeterminate;
    }
    for ( long int i = 0; i < intervals.size(); i++ ) {
        double entry = intervals[i].start;
        double exit = intervals[i].end;
        if ( entry >= query.minT && entry <= query.maxT ) {
            if ( !result.boundaries.add(RaySolidBoundary(entry, this, false,
                     intervals[i].startSurface)) ||
                 !result.insideSegments.add(true) ) {
                result.clear();
                return RaySolidQueryStatus::Indeterminate;
            }
        }
        if ( exit >= query.minT && exit <= query.maxT ) {
            if ( !result.boundaries.add(RaySolidBoundary(exit, this, false,
                     intervals[i].endSurface)) ||
                 !result.insideSegments.add(false) ) {
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

GeometryContainment
Prism::classifyPoint(const Vector3Dd& p, double distanceTolerance) const
{
    if ( !java::Double::isFinite(distanceTolerance) || distanceTolerance < 0 ||
         !java::Double::isFinite(p.x()) || !java::Double::isFinite(p.y()) ||
         !java::Double::isFinite(p.z()) ) {
        return GeometryContainment::InvalidInput;
    }
    if ( !hasValidParameters() ) {
        return GeometryContainment::InvalidInput;
    }
    int c = const_cast<Prism*>(this)->doContainmentTest(p, distanceTolerance);
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

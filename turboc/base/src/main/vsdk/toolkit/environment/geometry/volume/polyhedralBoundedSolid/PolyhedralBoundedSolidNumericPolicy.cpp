#include <stddef.h>
#include <math.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
#include "vsdk/toolkit/common/Cpp98Support.h"
#include "java/lang/Math.h"


// Same value as VSDK::EPSILON, spelled as a literal so that it is constant
// initialized (no static initialization order dependency on VSDK.cpp).
const double PolyhedralBoundedSolidNumericPolicy::BREP_EPSILON = 1e-6;
const double PolyhedralBoundedSolidNumericPolicy::BREP_BIG_EPSILON =
    10.0 * 1e-6;

// Scale of an empty or degenerate model (no extent to measure).
static const double DEFAULT_SCALE = 1.0;
// Smallest model scale. Tolerances follow the model down to this size, so
// a part modelled in metres at 10^-3 gets 10^-3 times the tolerances of
// the same part at unit size. They used to stop at scale 1: below it the
// tolerances were absolute and, at 10^-3, a tenth of the features.
static const double MIN_SCALE = 1.0e-9;
static const double MAX_UNIT_INTERVAL_TOLERANCE = 1.0e-3;

double PolyhedralBoundedSolidNumericPolicy::sanitizeScale(double scale)
{
    if ( !Cpp98Support::isFinite(scale) || scale <= 0.0 ) {
        return DEFAULT_SCALE;
    }
    if ( scale < MIN_SCALE ) {
        return MIN_SCALE;
    }
    return scale;
}

double PolyhedralBoundedSolidNumericPolicy::clamp(double value, double min,
                                                  double max)
{
    if ( value < min ) {
        return min;
    }
    if ( value > max ) {
        return max;
    }
    return value;
}

double PolyhedralBoundedSolidNumericPolicy::estimateSolidScale(
    PolyhedralBoundedSolid* solid)
{
    if ( solid == NULL ) {
        return DEFAULT_SCALE;
    }

    double* minMax = solid->getMinMax();
    if ( minMax == NULL ) {
        return DEFAULT_SCALE;
    }
    double scale = diagonalSize(minMax[0], minMax[1], minMax[2],
                                minMax[3], minMax[4], minMax[5]);
    delete[] minMax;
    return scale;
}

double PolyhedralBoundedSolidNumericPolicy::estimatePointsScale(
    const ArrayList<Vector3Dd>& points)
{
    if ( points.size() < 2 ) {
        return DEFAULT_SCALE;
    }

    double minX = HUGE_VAL;
    double minY = HUGE_VAL;
    double minZ = HUGE_VAL;
    double maxX = -HUGE_VAL;
    double maxY = -HUGE_VAL;
    double maxZ = -HUGE_VAL;
    BOOLEAN found = FALSE;

    for ( long i = 0; i < points.size(); i++ ) {
        const Vector3Dd& p = points[i];
        found = TRUE;
        if ( p.x() < minX ) minX = p.x();
        if ( p.y() < minY ) minY = p.y();
        if ( p.z() < minZ ) minZ = p.z();
        if ( p.x() > maxX ) maxX = p.x();
        if ( p.y() > maxY ) maxY = p.y();
        if ( p.z() > maxZ ) maxZ = p.z();
    }

    if ( !found ) {
        return DEFAULT_SCALE;
    }
    return diagonalSize(minX, minY, minZ, maxX, maxY, maxZ);
}

double PolyhedralBoundedSolidNumericPolicy::diagonalSize(double minX, double minY, double minZ,
                    double maxX, double maxY, double maxZ)
{
    double dx = maxX - minX;
    double dy = maxY - minY;
    double dz = maxZ - minZ;
    return sanitizeScale(sqrt(dx*dx + dy*dy + dz*dz));
}

double PolyhedralBoundedSolidNumericPolicy::estimateFaceScale(_PolyhedralBoundedSolidFace* face)
{
    if ( face == 0 ) {
        return DEFAULT_SCALE;
    }

    double minX = HUGE_VAL;
    double minY = HUGE_VAL;
    double minZ = HUGE_VAL;
    double maxX = -HUGE_VAL;
    double maxY = -HUGE_VAL;
    double maxZ = -HUGE_VAL;
    BOOLEAN found = FALSE;

    for ( long int i = 0; i < face->boundariesList.size(); ++i ) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        if ( loop == 0 ) {
            continue;
        }
        for ( long int j = 0; j < loop->halfEdgesList.size(); ++j ) {
            _PolyhedralBoundedSolidHalfEdge* he = loop->halfEdgesList.get(j);
            if ( he == 0 || he->startingVertex == 0 ) {
                continue;
            }
            Vector3Dd p = he->startingVertex->position;
            found = TRUE;
            if ( p.x() < minX ) minX = p.x();
            if ( p.y() < minY ) minY = p.y();
            if ( p.z() < minZ ) minZ = p.z();
            if ( p.x() > maxX ) maxX = p.x();
            if ( p.y() > maxY ) maxY = p.y();
            if ( p.z() > maxZ ) maxZ = p.z();
        }
    }

    if ( !found ) {
        return DEFAULT_SCALE;
    }
    return diagonalSize(minX, minY, minZ, maxX, maxY, maxZ);
}

PolyhedralBoundedSolidNumericPolicy::ToleranceContext::ToleranceContext()
{
    *this = PolyhedralBoundedSolidNumericPolicy::fromScale(DEFAULT_SCALE);
}

PolyhedralBoundedSolidNumericPolicy::ToleranceContext::ToleranceContext(double modelScale, double epsilon, double bigEpsilon,
    double unitVectorTolerance, double angleTolerance, double coplanarDotTolerance, double unitIntervalTolerance)
    : modelScale_(modelScale), epsilon_(epsilon), bigEpsilon_(bigEpsilon), unitVectorTolerance_(unitVectorTolerance),
      angleTolerance_(angleTolerance), coplanarDotTolerance_(coplanarDotTolerance), unitIntervalTolerance_(unitIntervalTolerance) {}

double PolyhedralBoundedSolidNumericPolicy::ToleranceContext::modelScale() const { return modelScale_; }
double PolyhedralBoundedSolidNumericPolicy::ToleranceContext::epsilon() const { return epsilon_; }
double PolyhedralBoundedSolidNumericPolicy::ToleranceContext::bigEpsilon() const { return bigEpsilon_; }
double PolyhedralBoundedSolidNumericPolicy::ToleranceContext::unitVectorTolerance() const { return unitVectorTolerance_; }
double PolyhedralBoundedSolidNumericPolicy::ToleranceContext::angleTolerance() const { return angleTolerance_; }
double PolyhedralBoundedSolidNumericPolicy::ToleranceContext::coplanarDotTolerance() const { return coplanarDotTolerance_; }
double PolyhedralBoundedSolidNumericPolicy::ToleranceContext::unitIntervalTolerance() const { return unitIntervalTolerance_; }

PolyhedralBoundedSolidNumericPolicy::ToleranceContext
PolyhedralBoundedSolidNumericPolicy::defaultContext()
{
    return fromScale(DEFAULT_SCALE);
}

PolyhedralBoundedSolidNumericPolicy::ToleranceContext
PolyhedralBoundedSolidNumericPolicy::fromScale(double modelScale)
{
    double safeScale = sanitizeScale(modelScale);
    double eps = BREP_EPSILON * safeScale;
    double bigEps = BREP_BIG_EPSILON * safeScale;
    double unitTol = BREP_BIG_EPSILON;
    double angleTol = BREP_BIG_EPSILON;
    double coplanarDotTol = 10.0 * BREP_BIG_EPSILON;
    double unitIntervalTol = clamp(bigEps / safeScale,
        BREP_BIG_EPSILON, MAX_UNIT_INTERVAL_TOLERANCE);
    return ToleranceContext(safeScale, eps, bigEps, unitTol, angleTol,
        coplanarDotTol, unitIntervalTol);
}

PolyhedralBoundedSolidNumericPolicy::ToleranceContext
PolyhedralBoundedSolidNumericPolicy::forSolid(PolyhedralBoundedSolid* solid)
{
    return fromScale(estimateSolidScale(solid));
}

PolyhedralBoundedSolidNumericPolicy::ToleranceContext
PolyhedralBoundedSolidNumericPolicy::forSolids(PolyhedralBoundedSolid* a,
                                               PolyhedralBoundedSolid* b)
{
    return fromScale(Math::max(estimateSolidScale(a), estimateSolidScale(b)));
}

PolyhedralBoundedSolidNumericPolicy::ToleranceContext
PolyhedralBoundedSolidNumericPolicy::forFace(_PolyhedralBoundedSolidFace* face)
{
    return fromScale(estimateFaceScale(face));
}

PolyhedralBoundedSolidNumericPolicy::ToleranceContext
PolyhedralBoundedSolidNumericPolicy::forPoints(
    const ArrayList<Vector3Dd>& points)
{
    return fromScale(estimatePointsScale(points));
}

int PolyhedralBoundedSolidNumericPolicy::compare(double a, double b,
                                                 double tolerance)
{
    return PolyhedralBoundedSolid::compareValue(a, b, tolerance);
}

int PolyhedralBoundedSolidNumericPolicy::compare(double a, double b,
    const ToleranceContext& context)
{
    return compare(a, b, context.epsilon());
}

int PolyhedralBoundedSolidNumericPolicy::compareToZero(double value,
    const ToleranceContext& context)
{
    return compare(value, 0.0, context.epsilon());
}

int PolyhedralBoundedSolidNumericPolicy::compareToZeroBig(double value,
    const ToleranceContext& context)
{
    return compare(value, 0.0, context.bigEpsilon());
}

BOOLEAN PolyhedralBoundedSolidNumericPolicy::isZero(double value,
    const ToleranceContext& context)
{
    return Cpp98Support::abs(value) <= context.epsilon();
}

BOOLEAN PolyhedralBoundedSolidNumericPolicy::isZeroBig(double value,
    const ToleranceContext& context)
{
    return Cpp98Support::abs(value) <= context.bigEpsilon();
}

BOOLEAN PolyhedralBoundedSolidNumericPolicy::pointsCoincident(const Vector3Dd& a,
    const Vector3Dd& b, const ToleranceContext& context)
{
    return a.subtract(b).length() <= context.bigEpsilon();
}

BOOLEAN PolyhedralBoundedSolidNumericPolicy::pointsSeparated(const Vector3Dd& a,
    const Vector3Dd& b, const ToleranceContext& context)
{
    return a.subtract(b).length() > context.bigEpsilon();
}

int PolyhedralBoundedSolidNumericPolicy::testPointInside(
    _PolyhedralBoundedSolidFace* face, const Vector3Dd& point,
    const ToleranceContext& context)
{
    return face->testPointInside(point, context.bigEpsilon());
}

BOOLEAN PolyhedralBoundedSolidNumericPolicy::vectorsColinear(const Vector3Dd& a,
    const Vector3Dd& b, const ToleranceContext& context)
{
    // |a x b| = |a| |b| sin(angle): compare the sine, so that the test
    // does not depend on the length of the vectors (it used to accept
    // angles up to bigEpsilon / length, half a degree at 10^-3).
    return a.crossProduct(b).length() <=
        BREP_BIG_EPSILON * a.length() * b.length();
}

BOOLEAN PolyhedralBoundedSolidNumericPolicy::unitVectorsParallel(
    const Vector3Dd& a, const Vector3Dd& b, const ToleranceContext& context)
{
    return a.crossProduct(b).length() <= context.unitVectorTolerance();
}

BOOLEAN PolyhedralBoundedSolidNumericPolicy::angleIntervalsOverlap(double upperA,
    double lowerB, const ToleranceContext& context)
{
    double tolerance = context.angleTolerance();
    return upperA + tolerance > lowerB - tolerance;
}

BOOLEAN PolyhedralBoundedSolidNumericPolicy::unitIntervalContainsStrictly(
    double t, const ToleranceContext& context)
{
    return t > context.unitIntervalTolerance() &&
           t < 1.0 - context.unitIntervalTolerance();
}

double PolyhedralBoundedSolidNumericPolicy::orientationTolerance2D(
    const Vector2Dd& a, const Vector2Dd& b, const Vector2Dd& c,
    const ToleranceContext& context)
{
    double lx = Math::max(Math::max(Cpp98Support::abs(a.x - b.x),
                                  Cpp98Support::abs(a.x - c.x)),
                         Cpp98Support::abs(b.x - c.x));
    double ly = Math::max(Math::max(Cpp98Support::abs(a.y - b.y),
                                  Cpp98Support::abs(a.y - c.y)),
                         Cpp98Support::abs(b.y - c.y));
    // The orientation is a cross product, |b - a| times the distance of
    // c from the line: a distance tolerance times the span. The span
    // used to be at least 1, which below unit size made the tolerance
    // an angle of bigEpsilon / span (half a degree at 10^-3).
    double span = Math::max(lx, ly);
    return context.bigEpsilon() * span;
}

double PolyhedralBoundedSolidNumericPolicy::linearTolerance2D(
    const ToleranceContext& context)
{
    return context.bigEpsilon();
}

double PolyhedralBoundedSolidNumericPolicy::areaTolerance2D(
    const ToleranceContext& context)
{
    double linearTolerance = context.bigEpsilon();
    return linearTolerance * linearTolerance;
}

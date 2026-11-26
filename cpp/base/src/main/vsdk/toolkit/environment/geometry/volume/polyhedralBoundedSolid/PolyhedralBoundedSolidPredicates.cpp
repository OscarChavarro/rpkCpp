#include "java/lang/Math.h"
#include <cmath>

#include "java/util/ArrayList.txx"
#include "java/util/Collections.h"
#include "java/util/Comparator.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidPredicates.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
namespace {

java::ArrayList<Vector3Dd> buildProbeDirections()
{
    java::ArrayList<Vector3Dd> probes;
    probes.add(Vector3Dd(0.3172, 0.5490, 0.7725).normalized());
    probes.add(Vector3Dd(0.8030, -0.1399, 0.5793).normalized());
    probes.add(Vector3Dd(-0.4211, 0.7022, 0.5735).normalized());
    probes.add(Vector3Dd(0.6110, 0.4561, -0.6470).normalized());
    probes.add(Vector3Dd(-0.5121, -0.3307, 0.7925).normalized());
    probes.add(Vector3Dd(0.2671, -0.8113, 0.5201).normalized());
    probes.add(Vector3Dd(-0.7402, 0.4517, 0.4983).normalized());
    return probes;
}

const java::ArrayList<Vector3Dd>& probeDirections()
{
    static const java::ArrayList<Vector3Dd> probes = buildProbeDirections();
    return probes;
}

bool containsApproxDistance(
    const java::ArrayList<double>& values,
    double value,
    double tolerance)
{
    for (long int i = 0; i < values.size(); ++i) {
        if (std::abs(values[i] - value) < tolerance) {
            return true;
        }
    }
    return false;
}

int countBoundaryCrossingsToInfinity(
    PolyhedralBoundedSolid* solid,
    const Vector3Dd& point,
    const Vector3Dd& direction,
    double maxT,
    double bigEps)
{
    Ray ray(point, direction);
    java::ArrayList<double> crossings;
    bool borrowedPlanes = solid->visibilityQueriesActive();

    for (long int i = 0; i < solid->getPolygonsList().size(); ++i) {
        if (!solid->queryRayReachesFace(
                point,
                direction.x(),
                direction.y(),
                direction.z(),
                maxT,
                static_cast<int>(i),
                bigEps)) {
            continue;
        }

        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        InfinitePlane* plane = solid->cachedFacePlane(static_cast<int>(i));
        if (face == 0 || plane == 0) {
            continue;
        }

        RayHit planeHit;
        if (!plane->doIntersectionFirstHit(ray, &planeHit) || planeHit.getRay() == 0) {
            if (!borrowedPlanes) delete plane;
            continue;
        }

        Ray hit = *(planeHit.getRay());
        hit = hit.withDirection(hit.getDirection().normalized());
        double t = hit.getT();
        if (t <= bigEps || t >= maxT) {
            if (!borrowedPlanes) delete plane;
            continue;
        }

        Vector3Dd pi = hit.getOrigin().add(hit.getDirection().multiply(t));
        int classification = face->testPointInside(pi, bigEps, plane);
        if (!borrowedPlanes) delete plane;

        if (classification == Geometry::LIMIT) {
            return -(static_cast<int>(crossings.size()) + 1);
        }
        if (classification != Geometry::INSIDE) {
            continue;
        }
        if (!containsApproxDistance(crossings, t, bigEps)) {
            crossings.add(t);
        }
    }

    return static_cast<int>(crossings.size());
}

java::ArrayList<double> collectBoundaryCrossings(
    PolyhedralBoundedSolid* solid,
    const Vector3Dd& eye,
    const Vector3Dd& direction,
    double reach,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& context)
{
    Ray ray(eye, direction);
    double bigEps = context.bigEpsilon();
    java::ArrayList<double> crossings;
    bool borrowedPlanes = solid->visibilityQueriesActive();

    for (long int i = 0; i < solid->getPolygonsList().size(); ++i) {
        if (!solid->queryRayReachesFace(
                eye,
                direction.x(),
                direction.y(),
                direction.z(),
                reach,
                static_cast<int>(i),
                bigEps)) {
            continue;
        }

        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        InfinitePlane* plane = solid->cachedFacePlane(static_cast<int>(i));
        if (face == 0 || plane == 0) {
            continue;
        }

        RayHit planeHit;
        if (!plane->doIntersectionFirstHit(ray, &planeHit) || planeHit.getRay() == 0) {
            if (!borrowedPlanes) delete plane;
            continue;
        }

        Ray hit = *(planeHit.getRay());
        hit = hit.withDirection(hit.getDirection().normalized());
        double t = hit.getT();
        if (t <= bigEps || t >= reach) {
            if (!borrowedPlanes) delete plane;
            continue;
        }

        Vector3Dd pi = hit.getOrigin().add(hit.getDirection().multiply(t));
        int classification = face->testPointInside(pi, bigEps, plane);
        if (!borrowedPlanes) delete plane;
        if (classification == Geometry::OUTSIDE) {
            continue;
        }
        if (!containsApproxDistance(crossings, t, bigEps)) {
            crossings.add(t);
        }
    }

    java::Collections::sort(crossings, java::NaturalOrderComparator<double>());
    return crossings;
}

} // namespace

bool PolyhedralBoundedSolidPredicates::isPointInside(
    PolyhedralBoundedSolid* solid,
    const Vector3Dd& point)
{
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext context =
        solid->queryToleranceContext();
    double bigEps = context.bigEpsilon();
    double maxT = context.modelScale() * 100.0 + 1.0;
    int lastCrossings = 0;

    const java::ArrayList<Vector3Dd>& probes = probeDirections();
    for (long int i = 0; i < probes.size(); ++i) {
        int crossings = countBoundaryCrossingsToInfinity(
            solid, point, probes[i], maxT, bigEps);
        if (crossings >= 0) {
            return (crossings & 1) == 1;
        }
        lastCrossings = -(crossings + 1);
    }

    return (lastCrossings & 1) == 1;
}

int PolyhedralBoundedSolidPredicates::classifyPoint(
    PolyhedralBoundedSolid* solid,
    const Vector3Dd& point,
    double distanceTolerance)
{
    double tolerance = java::Math::max(distanceTolerance,
        solid->queryToleranceContext().bigEpsilon());
    // C++ port: outside a visibility query the plane is new (see
    // cachedFacePlane)
    bool borrowedPlanes = solid->visibilityQueriesActive();

    for ( int i = 0; i < (int)solid->getPolygonsList().size(); i++ ) {
        if ( !solid->queryPointNearFace(point, i, tolerance) ) {
            continue;
        }
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        InfinitePlane* plane = solid->cachedFacePlane(i);
        if ( plane == nullptr ) {
            continue;
        }
        bool onBoundary = std::abs(plane->pointDistance(point)) <= tolerance &&
             face->testPointInside(point, tolerance, plane) !=
                 Geometry::OUTSIDE;
        if ( !borrowedPlanes ) delete plane;
        if ( onBoundary ) {
            return Geometry::LIMIT;
        }
    }
    return isPointInside(solid, point) ? Geometry::INSIDE : Geometry::OUTSIDE;
}

int PolyhedralBoundedSolidPredicates::quantitativeInvisibility(
    PolyhedralBoundedSolid* solid,
    const Vector3Dd& eye,
    const Vector3Dd& point)
{
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext context =
        solid->queryToleranceContext();
    double eps = context.epsilon();
    Vector3Dd direction = point.subtract(eye);
    double distance = direction.length();
    if (distance <= eps) {
        return 0;
    }

    direction = direction.multiply(1.0 / distance);
    double reach = distance - distance * 1.0e-3;
    java::ArrayList<double> crossings =
        collectBoundaryCrossings(solid, eye, direction, reach, context);
    if (crossings.isEmpty()) {
        return 0;
    }

    int qi = 0;
    bool previousInterior = false;
    double previousBound = 0.0;
    for (long int i = 0; i <= crossings.size(); ++i) {
        double upper = i < crossings.size() ? crossings[i] : reach;
        if (upper - previousBound > eps) {
            Vector3Dd mid = eye.add(
                direction.multiply((previousBound + upper) * 0.5));
            int state = classifyOnSegment(solid, mid, context);
            bool interior = state == INTERIOR;
            if (interior && !previousInterior) {
                qi++;
            }
            previousInterior = interior;
        }
        previousBound = upper;
    }

    return qi;
}

int PolyhedralBoundedSolidPredicates::classifyOnSegment(
    PolyhedralBoundedSolid* solid,
    const Vector3Dd& point,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& context)
{
    double bigEps = context.bigEpsilon();
    bool borrowedPlanes = solid->visibilityQueriesActive();

    for (long int i = 0; i < solid->getPolygonsList().size(); ++i) {
        if (!solid->queryPointNearFace(point, static_cast<int>(i), bigEps)) {
            continue;
        }

        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        InfinitePlane* plane = solid->cachedFacePlane(static_cast<int>(i));
        if (face == 0 || plane == 0) {
            continue;
        }

        bool onSurface =
            std::abs(plane->pointDistance(point)) < bigEps &&
            face->testPointInside(point, bigEps, plane) != Geometry::OUTSIDE;
        if (!borrowedPlanes) delete plane;

        if (onSurface) {
            return ON_SURFACE;
        }
    }

    return isPointInside(solid, point) ? INTERIOR : OUTSIDE;
}

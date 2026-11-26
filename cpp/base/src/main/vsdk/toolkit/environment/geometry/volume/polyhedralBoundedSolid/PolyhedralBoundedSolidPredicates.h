#ifndef __POLYHEDRAL_BOUNDED_SOLID_PREDICATES__
#define __POLYHEDRAL_BOUNDED_SOLID_PREDICATES__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
class PolyhedralBoundedSolid;

class PolyhedralBoundedSolidPredicates {
public:
    static bool isPointInside(
        PolyhedralBoundedSolid* solid,
        const Vector3Dd& point);

    /**
    Classifies a point against the solid, as `Geometry::doContainmentTest`:
    `LIMIT` if it lies on a face (closer than the tolerance to the plane of
    the face, and inside the face polygon with that tolerance), otherwise
    `INSIDE` or `OUTSIDE` as decided by the robust `isPointInside`.

    @param solid the solid to test against (object-space geometry)
    @param point the query point, in the solid's object space
    @param distanceTolerance distance to the boundary under which the point
    is classified as `LIMIT` (the numeric tolerance of the solid is used if
    it is larger)
    @return Geometry::INSIDE, Geometry::OUTSIDE or Geometry::LIMIT
    */
    static int classifyPoint(
        PolyhedralBoundedSolid* solid,
        const Vector3Dd& point,
        double distanceTolerance);

    static int quantitativeInvisibility(
        PolyhedralBoundedSolid* solid,
        const Vector3Dd& eye,
        const Vector3Dd& point);

private:
    static const int OUTSIDE = 0;
    static const int INTERIOR = 1;
    static const int ON_SURFACE = 2;

    static int classifyOnSegment(
        PolyhedralBoundedSolid* solid,
        const Vector3Dd& point,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& context);
};

#endif

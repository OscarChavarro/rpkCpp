//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#include <math.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.txx"
#include "java/lang/Math.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidFaceGeometry.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

PolyhedralBoundedSolidFaceGeometry::PolyhedralBoundedSolidFaceGeometry(
    _PolyhedralBoundedSolidFace* face)
{
    _PolyhedralBoundedSolidLoop* loop;
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidHalfEdge* start;
    double lowX = HUGE_VAL;
    double lowY = HUGE_VAL;
    double lowZ = HUGE_VAL;
    double highX = -HUGE_VAL;
    double highY = -HUGE_VAL;
    double highZ = -HUGE_VAL;
    double deviation = 0.0;
    double longest = 0.0;
    long int i;

    plane = face->getContainingPlane();
    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        loop = face->boundariesList.get(i);
        if ( loop == NULL || loop->boundaryStartHalfEdge == NULL ) {
            continue;
        }
        start = loop->boundaryStartHalfEdge;
        he = start;
        do {
            Vector3Dd p = he->startingVertex->position;
            lowX = Math::min(lowX, p.x());
            lowY = Math::min(lowY, p.y());
            lowZ = Math::min(lowZ, p.z());
            highX = Math::max(highX, p.x());
            highY = Math::max(highY, p.y());
            highZ = Math::max(highZ, p.z());
            if ( plane != NULL ) {
                deviation = Math::max(deviation,
                    fabs(plane->pointDistance(p)));
            }
            if ( he->next() == NULL ) {
                break;
            }
            longest = Math::max(longest,
                p.subtract(he->next()->startingVertex->position).length());
            he = he->next();
        } while ( he != start );
    }
    minX = lowX;
    minY = lowY;
    minZ = lowZ;
    maxX = highX;
    maxY = highY;
    maxZ = highZ;
    planeDeviation = deviation;
    longestEdge = longest;
    if ( plane == NULL ) {
        dominantCoordinate = 0;
    }
    else {
        dominantCoordinate = dominantCoordinateOf(plane->getNormal());
    }
}

PolyhedralBoundedSolidFaceGeometry::~PolyhedralBoundedSolidFaceGeometry()
{
    delete plane;
}

/** Same choice as `_PolyhedralBoundedSolidFace.testPointInsideDetailed`. */
int PolyhedralBoundedSolidFaceGeometry::dominantCoordinateOf(const Vector3Dd& n)
{
    if ( fabs(n.x()) >= fabs(n.y()) &&
         fabs(n.x()) >= fabs(n.z()) ) {
        return 1;
    }
    if ( fabs(n.y()) >= fabs(n.x()) &&
         fabs(n.y()) >= fabs(n.z()) ) {
        return 2;
    }
    return 3;
}

PolyhedralBoundedSolidFaceGeometry* PolyhedralBoundedSolidFaceGeometry::of(
    _PolyhedralBoundedSolidFace* face)
{
    PolyhedralBoundedSolidFaceGeometry* geometry;
    geometry = new PolyhedralBoundedSolidFaceGeometry(face);
    return geometry;
}

void PolyhedralBoundedSolidFaceGeometry::of(PolyhedralBoundedSolid* solid,
    ArrayList<PolyhedralBoundedSolidFaceGeometry*>& out)
{
    long int n = solid->getPolygonsList().size();
    long int i;

    out.clear();
    for ( i = 0; i < n; i++ ) {
        out.add(new PolyhedralBoundedSolidFaceGeometry(
            solid->getPolygonsList().get(i)));
    }
}

void PolyhedralBoundedSolidFaceGeometry::dispose(
    ArrayList<PolyhedralBoundedSolidFaceGeometry*>& geometry)
{
    long int i;

    for ( i = 0; i < geometry.size(); i++ ) {
        delete geometry[i];
    }
    geometry.clear();
}

/** The containing plane, or null for a degenerate face. */
InfinitePlane* PolyhedralBoundedSolidFaceGeometry::getPlane() const
{
    return plane;
}

double PolyhedralBoundedSolidFaceGeometry::getPlaneDeviation() const
{
    return planeDeviation;
}

/**
Margin that covers every tolerance of the point-in-face test: vertex
proximity (2 tolerance), edge proximity (tolerance) and the parametric
slack of the segment test (VSDK.EPSILON times the edge length).
*/
double PolyhedralBoundedSolidFaceGeometry::pointTestMargin(double tolerance) const
{
    return 4.0 * tolerance + 2.0 * VSDK::EPSILON * longestEdge;
}

/**
True when `point` is far enough from the face, in the two coordinates
kept by the point-in-face projection, that
`testPointInside(point, tolerance, getPlane())` must answer OUTSIDE.
*/
BOOLEAN PolyhedralBoundedSolidFaceGeometry::projectionExcludes(
    const Vector3Dd& point, double tolerance) const
{
    double m = pointTestMargin(tolerance);
    BOOLEAN xOut = point.x() < minX - m || point.x() > maxX + m;
    BOOLEAN yOut = point.y() < minY - m || point.y() > maxY + m;
    BOOLEAN zOut = point.z() < minZ - m || point.z() > maxZ + m;

    switch ( dominantCoordinate ) {
      case 1: return yOut || zOut;
      case 2: return xOut || zOut;
      case 3: return xOut || yOut;
      default: return FALSE;
    }
}

/**
True when the boxes of the two faces, each grown by `margin`, do not
overlap.
*/
BOOLEAN PolyhedralBoundedSolidFaceGeometry::boxSeparatedFrom(
    const PolyhedralBoundedSolidFaceGeometry* other, double margin) const
{
    return maxX + margin < other->minX - margin ||
           other->maxX + margin < minX - margin ||
           maxY + margin < other->minY - margin ||
           other->maxY + margin < minY - margin ||
           maxZ + margin < other->minZ - margin ||
           other->maxZ + margin < minZ - margin;
}

/**
True when the box of the segment from `from` to `to` and the box of
the face, each grown by `margin`, do not overlap.
*/
BOOLEAN PolyhedralBoundedSolidFaceGeometry::segmentSeparatedFrom(
    const Vector3Dd& from, const Vector3Dd& to, double margin) const
{
    return Math::max(from.x(), to.x()) + margin < minX - margin ||
           Math::min(from.x(), to.x()) - margin > maxX + margin ||
           Math::max(from.y(), to.y()) + margin < minY - margin ||
           Math::min(from.y(), to.y()) - margin > maxY + margin ||
           Math::max(from.z(), to.z()) + margin < minZ - margin ||
           Math::min(from.z(), to.z()) - margin > maxZ + margin;
}

/**
Margin to grow this face's box by so that it holds every point that a
tolerance-`tolerance` point-in-face test against it can accept, also in
the coordinate dropped by the projection (where a non-planar face can
reach `planeDeviation` beyond its plane; the factor 2 bounds the
sqrt(3) of a dominant normal component of at least 1/sqrt(3)).
*/
double PolyhedralBoundedSolidFaceGeometry::containmentMargin(double tolerance) const
{
    return pointTestMargin(tolerance) + 2.0 * (planeDeviation + tolerance);
}

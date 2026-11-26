//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#include "java/lang/Double.h"
#include "java/lang/Math.h"

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
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
    double lowX = java::Double::POSITIVE_INFINITY;
    double lowY = java::Double::POSITIVE_INFINITY;
    double lowZ = java::Double::POSITIVE_INFINITY;
    double highX = java::Double::NEGATIVE_INFINITY;
    double highY = java::Double::NEGATIVE_INFINITY;
    double highZ = java::Double::NEGATIVE_INFINITY;
    double deviation = 0.0;
    double longest = 0.0;
    int i;

    plane = face->getContainingPlane();
    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        loop = face->boundariesList.get(i);
        if ( loop == 0 || loop->boundaryStartHalfEdge == 0 ) {
            continue;
        }
        start = loop->boundaryStartHalfEdge;
        he = start;
        do {
            Vector3Dd p = he->startingVertex->position;
            lowX = java::Math::min(lowX, p.x());
            lowY = java::Math::min(lowY, p.y());
            lowZ = java::Math::min(lowZ, p.z());
            highX = java::Math::max(highX, p.x());
            highY = java::Math::max(highY, p.y());
            highZ = java::Math::max(highZ, p.z());
            if ( plane != 0 ) {
                deviation = java::Math::max(deviation,
                    java::Math::abs(plane->pointDistance(p)));
            }
            if ( he->next() == 0 ) {
                break;
            }
            longest = java::Math::max(longest, Vector3Dd::distance(p,
                he->next()->startingVertex->position));
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
    dominantCoordinate = plane == 0 ? 0 :
        dominantCoordinateOf(plane->getNormal());
}

PolyhedralBoundedSolidFaceGeometry::~PolyhedralBoundedSolidFaceGeometry()
{
    delete plane;
}

int PolyhedralBoundedSolidFaceGeometry::dominantCoordinateOf(const Vector3Dd& n)
{
    if ( java::Math::abs(n.x()) >= java::Math::abs(n.y()) &&
         java::Math::abs(n.x()) >= java::Math::abs(n.z()) ) {
        return 1;
    }
    if ( java::Math::abs(n.y()) >= java::Math::abs(n.x()) &&
         java::Math::abs(n.y()) >= java::Math::abs(n.z()) ) {
        return 2;
    }
    return 3;
}

PolyhedralBoundedSolidFaceGeometry* PolyhedralBoundedSolidFaceGeometry::of(
    _PolyhedralBoundedSolidFace* face)
{
    return new PolyhedralBoundedSolidFaceGeometry(face);
}

java::ArrayList<PolyhedralBoundedSolidFaceGeometry*>
PolyhedralBoundedSolidFaceGeometry::of(PolyhedralBoundedSolid* solid)
{
    int n = solid->getPolygonsList().size();
    java::ArrayList<PolyhedralBoundedSolidFaceGeometry*> out;
    int i;

    for ( i = 0; i < n; i++ ) {
        out.add(new PolyhedralBoundedSolidFaceGeometry(
            solid->getPolygonsList().get(i)));
    }
    return out;
}

void PolyhedralBoundedSolidFaceGeometry::release(
    java::ArrayList<PolyhedralBoundedSolidFaceGeometry*>& geometry)
{
    long int i;

    for ( i = 0; i < geometry.size(); i++ ) {
        delete geometry.get(i);
    }
    geometry.clear();
}

InfinitePlane* PolyhedralBoundedSolidFaceGeometry::getPlane() const
{
    return plane;
}

double PolyhedralBoundedSolidFaceGeometry::getPlaneDeviation() const
{
    return planeDeviation;
}

double PolyhedralBoundedSolidFaceGeometry::pointTestMargin(double tolerance) const
{
    return 4.0 * tolerance + 2.0 * VSDK::EPSILON * longestEdge;
}

bool PolyhedralBoundedSolidFaceGeometry::projectionExcludes(
    const Vector3Dd& point, double tolerance) const
{
    double m = pointTestMargin(tolerance);
    bool xOut = point.x() < minX - m || point.x() > maxX + m;
    bool yOut = point.y() < minY - m || point.y() > maxY + m;
    bool zOut = point.z() < minZ - m || point.z() > maxZ + m;

    switch ( dominantCoordinate ) {
      case 1: return yOut || zOut;
      case 2: return xOut || zOut;
      case 3: return xOut || yOut;
      default: return false;
    }
}

bool PolyhedralBoundedSolidFaceGeometry::boxSeparatedFrom(
    const PolyhedralBoundedSolidFaceGeometry* other, double margin) const
{
    return maxX + margin < other->minX - margin ||
           other->maxX + margin < minX - margin ||
           maxY + margin < other->minY - margin ||
           other->maxY + margin < minY - margin ||
           maxZ + margin < other->minZ - margin ||
           other->maxZ + margin < minZ - margin;
}

bool PolyhedralBoundedSolidFaceGeometry::segmentSeparatedFrom(
    const Vector3Dd& from, const Vector3Dd& to, double margin) const
{
    return java::Math::max(from.x(), to.x()) + margin < minX - margin ||
           java::Math::min(from.x(), to.x()) - margin > maxX + margin ||
           java::Math::max(from.y(), to.y()) + margin < minY - margin ||
           java::Math::min(from.y(), to.y()) - margin > maxY + margin ||
           java::Math::max(from.z(), to.z()) + margin < minZ - margin ||
           java::Math::min(from.z(), to.z()) - margin > maxZ + margin;
}

double PolyhedralBoundedSolidFaceGeometry::containmentMargin(double tolerance) const
{
    return pointTestMargin(tolerance) + 2.0 * (planeDeviation + tolerance);
}

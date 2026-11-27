#include <math.h>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/processing/ComputationalGeometry.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
#include "vsdk/toolkit/common/Cpp98Support.h"
double _PolyhedralBoundedSolidFace::boundaryLoopAreaMagnitude(
    _PolyhedralBoundedSolidLoop* loop)
{
    if (loop == 0 || loop->boundaryStartHalfEdge == 0) {
        return 0.0;
    }

    _PolyhedralBoundedSolidHalfEdge* start = loop->boundaryStartHalfEdge;
    _PolyhedralBoundedSolidHalfEdge* he = start;
    Vector3Dd normalAccumulator;
    do {
        Vector3Dd p = he->startingVertex->position;
        Vector3Dd q = he->next()->startingVertex->position;
        normalAccumulator = normalAccumulator.add(Vector3Dd(
            (p.y() - q.y()) * (p.z() + q.z()),
            (p.z() - q.z()) * (p.x() + q.x()),
            (p.x() - q.x()) * (p.y() + q.y())));
        he = he->next();
    } while (he != start);

    return normalAccumulator.length();
}

_PolyhedralBoundedSolidLoop* _PolyhedralBoundedSolidFace::selectLoopForPlaneCalculation(
    CircularArrayList<_PolyhedralBoundedSolidLoop*>& boundariesList)
{
    if (boundariesList.size() < 1) {
        return 0;
    }

    _PolyhedralBoundedSolidLoop* selectedLoop = boundariesList.get(0);
    double maxAreaMagnitude = boundaryLoopAreaMagnitude(selectedLoop);
    for (long int i = 1; i < boundariesList.size(); ++i) {
        _PolyhedralBoundedSolidLoop* candidate = boundariesList.get(i);
        double candidateAreaMagnitude = boundaryLoopAreaMagnitude(candidate);
        if (candidateAreaMagnitude > maxAreaMagnitude) {
            maxAreaMagnitude = candidateAreaMagnitude;
            selectedLoop = candidate;
        }
    }
    return selectedLoop;
}

InfinitePlane* _PolyhedralBoundedSolidFace::calculatePlaneByNewell(
    CircularArrayList<_PolyhedralBoundedSolidLoop*>& boundariesList,
    double tolerance)
{
    _PolyhedralBoundedSolidLoop* loop =
        selectLoopForPlaneCalculation(boundariesList);
    if (loop == 0 || loop->boundaryStartHalfEdge == 0) {
        return 0;
    }

    _PolyhedralBoundedSolidHalfEdge* he = loop->boundaryStartHalfEdge;
    _PolyhedralBoundedSolidHalfEdge* start = he;
    double nx = 0.0;
    double ny = 0.0;
    double nz = 0.0;
    double cx = 0.0;
    double cy = 0.0;
    double cz = 0.0;
    int count = 0;

    do {
        Vector3Dd p = he->startingVertex->position;
        Vector3Dd q = he->next()->startingVertex->position;
        nx += (p.y() - q.y()) * (p.z() + q.z());
        ny += (p.z() - q.z()) * (p.x() + q.x());
        nz += (p.x() - q.x()) * (p.y() + q.y());
        cx += p.x();
        cy += p.y();
        cz += p.z();
        ++count;
        he = he->next();
    } while (he != start);

    if (count < 3) {
        return 0;
    }

    Vector3Dd normal(nx, ny, nz);
    double length = normal.length();
    if (length <= tolerance) {
        return 0;
    }
    // The Newell normal measures twice the area. Vector3Dd.normalized()
    // leaves vectors shorter than VSDK.EPSILON unchanged, so a face of a
    // model at 10^-3 got a plane with a normal of length 10^-6 and every
    // distance to it shrank a millionfold. Normalize explicitly.
    normal = normal.multiply(1.0 / length);

    Vector3Dd centroid(cx / count, cy / count, cz / count);
    return new InfinitePlane(normal, centroid);
}

_PolyhedralBoundedSolidFace::_PolyhedralBoundedSolidFace(
    PolyhedralBoundedSolid* parent,
    int inId)
    : id(inId), parentSolid(parent)
{
}

_PolyhedralBoundedSolidFace::~_PolyhedralBoundedSolidFace()
{
    for (long int i = 0; i < boundariesList.size(); ++i) {
        delete boundariesList.get(i);
    }
}

_PolyhedralBoundedSolidHalfEdge* _PolyhedralBoundedSolidFace::findHalfEdge(
    int vn1,
    int vn2)
{
    for (long int i = 0; i < boundariesList.size(); ++i) {
        _PolyhedralBoundedSolidHalfEdge* he =
            boundariesList.get(i)->halfEdgeVertices(vn1, vn2);
        if (he != 0) return he;
    }
    return 0;
}

_PolyhedralBoundedSolidHalfEdge* _PolyhedralBoundedSolidFace::findHalfEdge(
    int vn1)
{
    for (long int i = 0; i < boundariesList.size(); ++i) {
        _PolyhedralBoundedSolidHalfEdge* he =
            boundariesList.get(i)->firstHalfEdgeAtVertex(vn1);
        if (he != 0) return he;
    }
    return 0;
}

BOOLEAN _PolyhedralBoundedSolidFace::calculatePlane()
{
    return !hasContainingPlane();
}

Vector3Dd _PolyhedralBoundedSolidFace::containingPlaneNormal()
{
    InfinitePlane* plane = getContainingPlane();
    Vector3Dd normal = plane->getNormal();
    delete plane;
    return normal;
}

BOOLEAN _PolyhedralBoundedSolidFace::hasContainingPlane()
{
    InfinitePlane* plane = getContainingPlane();
    BOOLEAN present = plane != NULL;
    delete plane;
    return present;
}

InfinitePlane* _PolyhedralBoundedSolidFace::getContainingPlane()
{
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext numericContext =
        PolyhedralBoundedSolidNumericPolicy::forFace(this);
    InfinitePlane* plane =
        calculatePlaneByNewell(boundariesList, numericContext.bigEpsilon());
    if (plane == 0) {
        plane = calculatePlaneByCorner(numericContext.bigEpsilon());
    }
    return plane;
}

int _PolyhedralBoundedSolidFace::testPointInside(
    const Vector3Dd& p,
    double tolerance)
{
    return testPointInsideDetailed(p, tolerance).status();
}

int _PolyhedralBoundedSolidFace::testPointInside(
    const Vector3Dd& p,
    double tolerance,
    InfinitePlane* plane)
{
    return testPointInsideDetailed(p, tolerance, plane).status();
}

_PolyhedralBoundedSolidFace::PointInsideResult
_PolyhedralBoundedSolidFace::testPointInsideDetailed(
    const Vector3Dd& p,
    double tolerance)
{
    InfinitePlane* plane = getContainingPlane();
    PointInsideResult result = testPointInsideDetailed(p, tolerance, plane);
    delete plane;
    return result;
}

_PolyhedralBoundedSolidFace::PointInsideResult
_PolyhedralBoundedSolidFace::testPointInsideDetailed(
    const Vector3Dd& p,
    double tolerance,
    InfinitePlane* plane)
{
    int nc;
    int sh;
    int nsh;
    ArrayList<double> polygon2Du;
    ArrayList<double> polygon2Dv;
    ArrayList<_PolyhedralBoundedSolidVertex*> polygon2Dvv;
    Vector3Dd projectedPoint;
    int dominantCoordinate;

    if (plane == 0) {
        return PointInsideResult(Geometry::OUTSIDE, 0, 0);
    }
    Vector3Dd n = plane->getNormal();

    if (Cpp98Support::abs(n.x()) >= Cpp98Support::abs(n.y()) &&
        Cpp98Support::abs(n.x()) >= Cpp98Support::abs(n.z())) {
        dominantCoordinate = 1;
    }
    else if (Cpp98Support::abs(n.y()) >= Cpp98Support::abs(n.x()) &&
             Cpp98Support::abs(n.y()) >= Cpp98Support::abs(n.z())) {
        dominantCoordinate = 2;
    }
    else {
        dominantCoordinate = 3;
    }

    long int i;
    for (i = 0; i < boundariesList.size(); ++i) {
        _PolyhedralBoundedSolidLoop* loop = boundariesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* he =
            loop != 0 ? loop->boundaryStartHalfEdge : 0;
        if (he == 0) {
            return PointInsideResult(Geometry::OUTSIDE, 0, 0);
        }
        _PolyhedralBoundedSolidHalfEdge* heStart = he;

        do {
            if (p.subtract(he->startingVertex->position).length() <
                2 * tolerance) {
                return PointInsideResult(
                    Geometry::LIMIT, 0, he->startingVertex);
            }

            projectedPoint = dropCoordinate(
                he->startingVertex->position, dominantCoordinate);
            polygon2Du.add(projectedPoint.x());
            polygon2Dv.add(projectedPoint.y());
            polygon2Dvv.add(he->startingVertex);

            _PolyhedralBoundedSolidHalfEdge* heOld = he;
            he = he->next();
            if (he == 0) {
                return PointInsideResult(Geometry::OUTSIDE, 0, 0);
            }

            projectedPoint = dropCoordinate(
                he->startingVertex->position, dominantCoordinate);
            polygon2Du.add(projectedPoint.x());
            polygon2Dv.add(projectedPoint.y());
            polygon2Dvv.add(he->startingVertex);

            if (p.subtract(he->startingVertex->position).length() <
                2 * tolerance) {
                return PointInsideResult(
                    Geometry::LIMIT, 0, he->startingVertex);
            }

            if (ComputationalGeometry::lineSegmentContainmentTest(
                    heOld->startingVertex->position,
                    he->startingVertex->position,
                    p,
                    tolerance) == Geometry::LIMIT) {
                return PointInsideResult(Geometry::LIMIT, heOld, 0);
            }
        } while (he != heStart);
    }

    projectedPoint = dropCoordinate(p, dominantCoordinate);
    double u = projectedPoint.x();
    double v = projectedPoint.y();

    for (i = 0; i < polygon2Du.size(); ++i) {
        polygon2Du.set(i, polygon2Du.get(i) - u);
        polygon2Dv.set(i, polygon2Dv.get(i) - v);
    }
    nc = 0;

    for (i = 0; i < polygon2Du.size() - 1; i += 2) {
        double ua = polygon2Du.get(i);
        double va = polygon2Dv.get(i);
        double ub = polygon2Du.get(i + 1);
        double vb = polygon2Dv.get(i + 1);

        sh = va < 0 ? -1 : 1;
        nsh = vb < 0 ? -1 : 1;

        if (sh != nsh) {
            if (ua >= 0 && ub >= 0) {
                nc++;
            }
            else if (ua >= 0 || ub >= 0) {
                if (ua - va * (ub - ua) / (vb - va) > 0) {
                    nc++;
                }
            }
        }
    }

    if ((nc % 2) == 1) {
        return PointInsideResult(Geometry::INSIDE, 0, 0);
    }
    return PointInsideResult(Geometry::OUTSIDE, 0, 0);
}

InfinitePlane* _PolyhedralBoundedSolidFace::calculatePlaneByCorner(
    double tolerance)
{
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext numericContext =
        PolyhedralBoundedSolidNumericPolicy::forFace(this);
    double nonColinearDotTolerance = numericContext.coplanarDotTolerance();
    _PolyhedralBoundedSolidLoop* loop;
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidHalfEdge* heStart;
    _PolyhedralBoundedSolidHalfEdge* heInferior;
    Vector3Dd p0;
    Vector3Dd p1;
    Vector3Dd a;
    Vector3Dd b;
    Vector3Dd n1;
    Vector3Dd temp;
    BOOLEAN readyVecA;
    BOOLEAN readyVecB;
    double dotP;
    unsigned char domPlane;
    Vector3Dd vPrev;
    Vector3Dd vNext;

    if (boundariesList.size() < 1) {
        return 0;
    }
    loop = selectLoopForPlaneCalculation(boundariesList);
    if (loop == 0) {
        return 0;
    }
    he = loop->boundaryStartHalfEdge;
    if (he == 0) {
        return 0;
    }
    heStart = he;

    readyVecA = FALSE;
    readyVecB = FALSE;

    do {
        p0 = he->startingVertex->position;
        p1 = he->next()->startingVertex->position;
        temp = p1.subtract(p0);
        if (!readyVecA) {
            if (temp.length() > tolerance) {
                a = temp.normalized();
                readyVecA = TRUE;
            }
        }
        else if (!readyVecB) {
            if (temp.length() > tolerance) {
                temp = temp.normalized();
                dotP = Cpp98Support::abs(temp.dotProduct(a));
                if (dotP < 1 - nonColinearDotTolerance) {
                    b = temp;
                    readyVecB = TRUE;
                }
            }
        }
        he = he->next();
    } while (he != heStart && !readyVecB);

    if (a.length() == 0 || b.length() == 0) {
        return 0;
    }

    n1 = a.crossProduct(b);
    if (loop->halfEdgesList.size() == 3) {
        return new InfinitePlane(n1.normalized(), p0);
    }

    if (Cpp98Support::abs(n1.z()) > Cpp98Support::abs(n1.x())) {
        if (Cpp98Support::abs(n1.z()) > Cpp98Support::abs(n1.y())) {
            domPlane = 1;
        }
        else {
            domPlane = 2;
        }
    }
    else if (Cpp98Support::abs(n1.x()) > Cpp98Support::abs(n1.y())) {
        domPlane = 3;
    }
    else {
        domPlane = 2;
    }

    he = loop->boundaryStartHalfEdge;
    heInferior = he;
    he = he->next();
    while (he != heStart) {
        p0 = he->startingVertex->position;
        switch (domPlane) {
          case 1:
            if (p0.y() < heInferior->startingVertex->position.y()) {
                heInferior = he;
            }
            break;
          case 2:
            if (p0.z() < heInferior->startingVertex->position.z()) {
                heInferior = he;
            }
            break;
          case 3:
            if (p0.z() < heInferior->startingVertex->position.z()) {
                heInferior = he;
            }
            break;
        }
        he = he->next();
    }

    he = heInferior;
    p0 = heInferior->startingVertex->position;
    do {
        he = he->next();
        p1 = he->startingVertex->position;
        vNext = p1.subtract(p0);
        if (vNext.length() > tolerance) {
            vNext = vNext.normalized();
            break;
        }
    } while (he != heInferior);

    he = heInferior;
    do {
        he = he->previous();
        p1 = he->startingVertex->position;
        vPrev = p1.subtract(p0);
        if (vPrev.length() > tolerance) {
            vPrev = vPrev.normalized();
            dotP = Cpp98Support::abs(vPrev.dotProduct(vNext));
            if (dotP < 1 - nonColinearDotTolerance) {
                break;
            }
        }
    } while (he != heInferior);

    n1 = vNext.crossProduct(vPrev);
    return new InfinitePlane(n1, p0);
}

Vector3Dd _PolyhedralBoundedSolidFace::dropCoordinate(
    const Vector3Dd& in,
    int coord) const
{
    switch (coord) {
      case 1:
        return Vector3Dd(in.y(), in.z(), 0);
      case 2:
        return Vector3Dd(in.x(), in.z(), 0);
      case 3:
      default:
        return Vector3Dd(in.x(), in.y(), 0);
    }
}

void _PolyhedralBoundedSolidFace::revert()
{
    for (long int i = 0; i < boundariesList.size(); ++i) {
        if (boundariesList.get(i) != 0) boundariesList.get(i)->revert();
    }
}

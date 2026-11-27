#ifndef __PBS_FACE__
#define __PBS_FACE__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/dataStructures/ArrayLinkable.h"
#include "vsdk/toolkit/common/dataStructures/CircularArrayList.h"
// The loop list needs the loop class complete (see CircularArrayList)
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class PolyhedralBoundedSolid;
class InfinitePlane;
class _PolyhedralBoundedSolidLoop;
class _PolyhedralBoundedSolidHalfEdge;
class _PolyhedralBoundedSolidVertex;

class _PolyhedralBoundedSolidFace : public ArrayLinkable {
public:
    class PointInsideResult {
    private:
        int status_;
        _PolyhedralBoundedSolidHalfEdge* intersectedHalfedge_;
        _PolyhedralBoundedSolidVertex* intersectedVertex_;

    public:
        PointInsideResult(
            int status,
            _PolyhedralBoundedSolidHalfEdge* intersectedHalfedge,
            _PolyhedralBoundedSolidVertex* intersectedVertex)
            : status_(status),
              intersectedHalfedge_(intersectedHalfedge),
              intersectedVertex_(intersectedVertex)
        {
        }

        int status() const { return status_; }
        _PolyhedralBoundedSolidHalfEdge* intersectedHalfedge() const
        {
            return intersectedHalfedge_;
        }
        _PolyhedralBoundedSolidVertex* intersectedVertex() const
        {
            return intersectedVertex_;
        }
    };

    int id;
    PolyhedralBoundedSolid* parentSolid;
    CircularArrayList<_PolyhedralBoundedSolidLoop*> boundariesList;

    _PolyhedralBoundedSolidFace(PolyhedralBoundedSolid* parent, int id);
    ~_PolyhedralBoundedSolidFace();

    _PolyhedralBoundedSolidHalfEdge* findHalfEdge(int vn1, int vn2);
    _PolyhedralBoundedSolidHalfEdge* findHalfEdge(int vn1);
    BOOLEAN calculatePlane();
    InfinitePlane* getContainingPlane();
    // C++ port: Java's getContainingPlane().getNormal() and
    // getContainingPlane() == null, without leaking the plane the call returns
    Vector3Dd containingPlaneNormal();
    BOOLEAN hasContainingPlane();
    int testPointInside(const Vector3Dd& point, double tolerance);
    int testPointInside(
        const Vector3Dd& point,
        double tolerance,
        InfinitePlane* plane);
    PointInsideResult testPointInsideDetailed(
        const Vector3Dd& point,
        double tolerance);
    PointInsideResult testPointInsideDetailed(
        const Vector3Dd& point,
        double tolerance,
        InfinitePlane* plane);
    void revert();

    /**
    @return a description of the node, as Java `toString`
    */
    String toString() const;

private:
    static double boundaryLoopAreaMagnitude(_PolyhedralBoundedSolidLoop* loop);
    static _PolyhedralBoundedSolidLoop* selectLoopForPlaneCalculation(
        CircularArrayList<_PolyhedralBoundedSolidLoop*>& boundariesList);
    static InfinitePlane* calculatePlaneByNewell(
        CircularArrayList<_PolyhedralBoundedSolidLoop*>& boundariesList,
        double tolerance);
    InfinitePlane* calculatePlaneByCorner(double tolerance);
    Vector3Dd dropCoordinate(const Vector3Dd& in, int coord) const;
};

#endif

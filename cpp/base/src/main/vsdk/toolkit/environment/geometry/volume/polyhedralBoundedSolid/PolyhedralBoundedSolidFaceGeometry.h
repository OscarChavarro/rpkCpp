//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef __POLYHEDRAL_BOUNDED_SOLID_FACE_GEOMETRY__
#define __POLYHEDRAL_BOUNDED_SOLID_FACE_GEOMETRY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class InfinitePlane;
class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidFace;

/**
Snapshot of the geometry of one face for read-only passes that query the
same unchanged faces many times (strict validation, point classification):
the containing plane, computed once, and the axis-aligned bounding box of
the face vertices (Fable5.2Analysis.md Stage G).

Faces do not cache their plane, because vertex positions are public and
change outside the Euler operators (transformations, snapping). A snapshot
is only valid while its solid is not changed; build it at the start of the
pass and drop it at the end.

The box tests are conservative with respect to
`_PolyhedralBoundedSolidFace::testPointInside(point, tolerance, plane)`:
`projectionExcludes` answers true only when that test would answer
OUTSIDE, so skipping the test then changes no result.

C++ port notes: the snapshot owns its plane. The Java array of snapshots
of a solid is a `java::ArrayList` of pointers owned by the caller, who
frees them with `release`.
*/
class PolyhedralBoundedSolidFaceGeometry {
private:
    InfinitePlane* plane;
    int dominantCoordinate;
    double minX;
    double minY;
    double minZ;
    double maxX;
    double maxY;
    double maxZ;
    /// Largest distance from a face vertex to the plane.
    double planeDeviation;
    double longestEdge;

    explicit PolyhedralBoundedSolidFaceGeometry(_PolyhedralBoundedSolidFace* face);
    PolyhedralBoundedSolidFaceGeometry(const PolyhedralBoundedSolidFaceGeometry& other);
    PolyhedralBoundedSolidFaceGeometry& operator=(const PolyhedralBoundedSolidFaceGeometry& other);

    /** Same choice as `_PolyhedralBoundedSolidFace::testPointInsideDetailed`. */
    static int dominantCoordinateOf(const Vector3Dd& n);

    /**
    Margin that covers every tolerance of the point-in-face test: vertex
    proximity (2 tolerance), edge proximity (tolerance) and the parametric
    slack of the segment test (VSDK::EPSILON times the edge length).
    */
    double pointTestMargin(double tolerance) const;

public:
    ~PolyhedralBoundedSolidFaceGeometry();

    /** @return a new snapshot, owned by the caller */
    static PolyhedralBoundedSolidFaceGeometry* of(
        _PolyhedralBoundedSolidFace* face);

    /**
    One snapshot per face, indexed like `solid->getPolygonsList()`.
    @return snapshots owned by the caller (see `release`)
    */
    static java::ArrayList<PolyhedralBoundedSolidFaceGeometry*> of(
        PolyhedralBoundedSolid* solid);

    /**
    C++ port note: frees the snapshots returned by `of(solid)`, as the
    garbage collector does in Java.
    */
    static void release(
        java::ArrayList<PolyhedralBoundedSolidFaceGeometry*>& geometry);

    /** The containing plane, or null for a degenerate face. */
    InfinitePlane* getPlane() const;

    double getPlaneDeviation() const;

    /**
    True when `point` is far enough from the face, in the two coordinates
    kept by the point-in-face projection, that
    `testPointInside(point, tolerance, getPlane())` must answer OUTSIDE.
    */
    bool projectionExcludes(const Vector3Dd& point, double tolerance) const;

    /**
    True when the boxes of the two faces, each grown by `margin`, do not
    overlap.
    */
    bool boxSeparatedFrom(const PolyhedralBoundedSolidFaceGeometry* other,
        double margin) const;

    /**
    True when the box of the segment from `from` to `to` and the box of
    the face, each grown by `margin`, do not overlap.
    */
    bool segmentSeparatedFrom(const Vector3Dd& from, const Vector3Dd& to,
        double margin) const;

    /**
    Margin to grow this face's box by so that it holds every point that a
    tolerance-`tolerance` point-in-face test against it can accept, also in
    the coordinate dropped by the projection (where a non-planar face can
    reach `planeDeviation` beyond its plane; the factor 2 bounds the
    sqrt(3) of a dominant normal component of at least 1/sqrt(3)).
    */
    double containmentMargin(double tolerance) const;
};

#endif

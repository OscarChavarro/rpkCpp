//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef __PBS_FACE_GEOMETRY__
#define __PBS_FACE_GEOMETRY__

#include "vsdk/toolkit/common/VSDK.h"
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

<p>Faces do not cache their plane, because vertex positions are public and
change outside the Euler operators (transformations, snapping). A snapshot
is only valid while its solid is not changed; build it at the start of the
pass and drop it at the end.</p>

<p>The box tests are conservative with respect to
`_PolyhedralBoundedSolidFace.testPointInside(point, tolerance, plane)`:
{@link #projectionExcludes} answers true only when that test would answer
OUTSIDE, so skipping the test then changes no result.</p>

C++ port note: the snapshot owns its plane. `of(solid, out)` fills `out`
with snapshots owned by the caller, released with `dispose(out)`.
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
    /** Largest distance from a face vertex to the plane. */
    double planeDeviation;
    double longestEdge;

    explicit PolyhedralBoundedSolidFaceGeometry(_PolyhedralBoundedSolidFace* face);
    PolyhedralBoundedSolidFaceGeometry(const PolyhedralBoundedSolidFaceGeometry& other);
    PolyhedralBoundedSolidFaceGeometry& operator=(const PolyhedralBoundedSolidFaceGeometry& other);

    static int dominantCoordinateOf(const Vector3Dd& n);
    double pointTestMargin(double tolerance) const;

public:
    ~PolyhedralBoundedSolidFaceGeometry();

    static PolyhedralBoundedSolidFaceGeometry* of(
        _PolyhedralBoundedSolidFace* face);

    /** One snapshot per face, indexed like `solid.getPolygonsList()`. */
    static void of(PolyhedralBoundedSolid* solid,
        ArrayList<PolyhedralBoundedSolidFaceGeometry*>& out);

    /** Deletes the snapshots of `geometry` and empties it. */
    static void dispose(ArrayList<PolyhedralBoundedSolidFaceGeometry*>& geometry);

    /** The containing plane, or NULL for a degenerate face. */
    InfinitePlane* getPlane() const;

    double getPlaneDeviation() const;

    BOOLEAN projectionExcludes(const Vector3Dd& point, double tolerance) const;

    BOOLEAN boxSeparatedFrom(const PolyhedralBoundedSolidFaceGeometry* other,
        double margin) const;

    BOOLEAN segmentSeparatedFrom(const Vector3Dd& from, const Vector3Dd& to,
        double margin) const;

    double containmentMargin(double tolerance) const;
};

#endif
